from psychopy import visual, core
import numpy as np
import datetime as dt
import serial, os, time
from serial.tools import list_ports
import matplotlib.pyplot as plt

# Windows
if os.name == 'nt':
    import msvcrt

# Posix (Linux, OS X)
else:
    import sys
    import termios
    import atexit
    from select import select


class KBHit:
    
    def __init__(self):
        '''Creates a KBHit object that you can call to do various keyboard things.
        '''
        if os.name == 'nt':
            pass
        
        else:
            # Save the terminal settings
            self.fd = sys.stdin.fileno()
            self.new_term = termios.tcgetattr(self.fd)
            self.old_term = termios.tcgetattr(self.fd)
    
            # New terminal setting unbuffered
            self.new_term[3] = (self.new_term[3] & ~termios.ICANON & ~termios.ECHO)
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.new_term)
    
            # Support normal-terminal reset at exit
            atexit.register(self.set_normal_term)
    
    def set_normal_term(self):
        ''' Resets to normal terminal.  On Windows this is a no-op.
        '''
        if os.name == 'nt':
            pass
        
        else:
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old_term)

    def getch(self):
        ''' Returns a keyboard character after kbhit() has been called.
            Should not be called in the same program as getarrow().
        '''
        s = ''
        if os.name == 'nt':
            return msvcrt.getch().decode('utf-8')
        
        else:
            return sys.stdin.read(1)
                        

    def getarrow(self):
        ''' Returns an arrow-key code after kbhit() has been called. Codes are
        0 : up
        1 : right
        2 : down
        3 : left
        Should not be called in the same program as getch().
        '''
        if os.name == 'nt':
            msvcrt.getch() # skip 0xE0
            c = msvcrt.getch()
            vals = [72, 77, 80, 75]
        else:
            c = sys.stdin.read(3)[2]
            vals = [65, 67, 66, 68]
        return vals.index(ord(c.decode('utf-8')))

    def kbhit(self):
        ''' Returns True if keyboard character was hit, False otherwise.
        '''
        if os.name == 'nt':
            return msvcrt.kbhit()
        
        else:
            dr,dw,de = select([sys.stdin], [], [], 0)
            return dr != []

#%% Functions
def onoffVisual(cVal1, cVal2):
    gabor.contrast = cVal1
    line1.contrast = cVal2
    line2.contrast = cVal2
    win.flip() # Flip image onto screen

def checkArduino(sflag):
    # waiting = arduino.in_waiting  # find num of bytes currently waiting in hardware
    # buffer += [chr(c) for c in port.read(waiting)] # read them, convert to ascii
    data = arduino.read(size=1)
    if data:
        if data is 'Q':
            useOri = 60#vStimOri[int(data)]
            gabor.pos = (np.sin(np.pi*useOri/180)*.35, np.cos(np.pi*useOri/180)*.35)
            gabor.ori = useOri + 270 # Grating orthogonal to positional angle
            onoffVisual(1,1)
            sflag = True
            win.flip()
        elif data is 'P':
            onoffVisual(0,1)
            sflag = False
            win.flip()
        f.write(data)
        return sflag

def serInitialization():
    # Set up arduino serial connection
    bRate = 28800 # Baudrate MUST match arduino
    ports = list_ports.comports(include_links=False)
    print('Available COM ports:')
    for port in ports:
        print(port.device)
    print('Make sure it selects the correct serial bus for the arduino!')

    #activate the serial port, if possible
    try:
        arduino = serial.Serial(ports[1].device, bRate, timeout = 0)
        print "'Python-arduino serial communication initialized successfully" 
        return arduino
    except:
        print "Error: serial port cannot be initialized"
        quit()

arduino = serInitialization()

#%% Setup: info and file handling
# Set up info for the data file header
info = {
    "experimenter" : "",
    "mouse" : "",
    "datetime" : ""
}

# Ask for information (mouse, experimenter, date automatic, training level)
info['experimenter'] = raw_input('Experimenter ID:')
info['mouse'] = raw_input('Mouse ID:')
info['datetime'] = dt.datetime.now().strftime("%Y%m%d%H%M")

# Detect the current working directory and print it
path = os.getcwd()  
print ("The current working directory is %s" % path)  

# Check if mouse directory exists, otherwise make it
if not os.path.exists(info['mouse']):
    os.mkdir(info['mouse'])

    # Enter mouse directory
os.chdir(path + '/' + info['mouse'])

# Open text file to write
filename = info['mouse'] + '_' + info['datetime'] + '.txt'
f = open(filename,'a+')

f.write("Experimenter: " + info['experimenter'] + "\n" + 'Datetime: ' + info['datetime'] + '\n' + 'MouseID: ' + info['mouse'] + '\n')

sflag = False

#%% Setup: Serial communication and presentation window
# Create visual stimulation window
win = visual.Window(size=(512,512), winType='pyglet', screen=0, fullscr=False, units='height')

# Generate Gabor stimulus
gabor = visual.GratingStim(win, tex='sin', mask='gauss', sf=6, name='gabor', size=0.35)
gabor.setAutoDraw(True)  # Automatically draw every frame
gabor.autoLog = False # Turn off messages about phase changes (or it will go crazy)

# Generate the visual stimulus set (6)
vStimOri = [0,60,120,180,240,300]

# Generate center reference cross in visual stimulus
line1 = visual.Line(win, start=(-.05,0), end=(.05,0))
line1.setAutoDraw(True)
line1.autoLog = False # Turn off messages

line2 = visual.Line(win, start=(0,-.05), end=(0,.05))
line2.setAutoDraw(True)
line2.autoLog = False # Turn off messages

onoffVisual(0,1) # Initialize with gray window
print('Visual presentation window initialized successfully')


# Set up protocol ready for start
doRun = False
print('\n............\nWaiting for spacebar to start/pause training\nEsc x 2 to quit protocol completely')

msvcrt = KBHit()

# Wait for key press
while not doRun:
    if msvcrt.kbhit():
            keyMe = msvcrt.getch()
            if ord(keyMe) == 32:
                arduino.flush()
                arduino.write('S')
                doRun = True
                print "Starting behavioural protocol"



#%% Run main loop
# Run stimulus presentation 
tStim = 1 # Stimulus presentation duration
counter = 0
while doRun:
    # Get arduino data
    sflag = checkArduino(sflag)

    if sflag:
        gabor.setPhase(0.1, '+')
        win.flip()
    
    # Check for keypresses
    if msvcrt.kbhit():
        keyMe = msvcrt.getch()
        # Pausing
        if ord(keyMe) is 32:
            print "Pausing behavioural protocol (spacebar to continue)"
            arduino.flush()
            arduino.write('S')
            onoffVisual(0,1)
            keyMe = msvcrt.getch()
            while ord(keyMe) is not 32:
                keyMe = msvcrt.getch()
            arduino.flush()
            arduino.write('S')
            print "Continuing behavioural protocol"
            
        # Quitting
        if ord(keyMe) is 27:
            print('Press esc again to quit or any other key to keep running')
            onoffVisual(0,1)
            keyMe = msvcrt.getch()
            if ord(keyMe) is 27:
                print "Quitting behavioural protocol"
                f.close()
                os.chdir(path)
                arduino.flush()
                arduino.write('S')
                time.sleep(.25)
                arduino.close()
                quit()
            else:
                print "Show goes on"