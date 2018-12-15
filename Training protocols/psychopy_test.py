from psychopy import visual, core
import numpy as np
import datetime as dt
import serial, os, msvcrt, time
from serial.tools import list_ports
import matplotlib.pyplot as plt


#%% Functions
def onoffVisual(cVal1, cVal2):
    gabor.contrast = cVal1
    line1.contrast = cVal2
    line2.contrast = cVal2
    win.flip() # Flip image onto screen

def checkArduino():
    data = arduino.readline()
    if data:
        if data[1:3] == 'S1':
            useOri = vStimOri[int(data[3])]
            print(data[3:4])
            gabor.pos = (np.sin(np.pi*useOri/180)*.35, np.cos(np.pi*useOri/180)*.35)
            gabor.ori = useOri + 270 # Grating orthogonal to positional angle
            onoffVisual(1,1)
            win.flip()
        elif data[1:3] == 'S2':
            onoffVisual(0,1)
            win.flip()
        f.write(data)



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



#%% Setup: Serial communication and presentation window
# Create visual stimulation window
win = visual.Window(size=(512,512), winType='pyglet', screen=1, fullscr=True, units='height')

# Generate Gabor stimulus
gabor = visual.GratingStim(win, tex='sin', mask='gauss', sf=6, name='gabor', size=0.35)
gabor.setAutoDraw(True)  # Automatically draw every frame
gabor.autoLog = False # Turn off messages about phase changes (or it will go crazy)

# Generate the visual stimulus set (6)
vStimOri = [0,60,120,180,240,300]
# Have a random shuffle per animal makes sense. How to save across sessions?
# vStimP = np.zeros([6,4])
# vStimP[[0,1,2,3,4,5],[0,1,1,2,2,3]] = 0.9
# vStimP[[0,1,2,3,4,5],[1,0,2,1,3,2]] = 0.1

# Generate center reference cross in visual stimulus
line1 = visual.Line(win, start=(-.05,0), end=(.05,0))
line1.setAutoDraw(True)
line1.autoLog = False # Turn off messages

line2 = visual.Line(win, start=(0,-.05), end=(0,.05))
line2.setAutoDraw(True)
line2.autoLog = False # Turn off messages

onoffVisual(0,0) # Initialize with gray window
print('Visual presentation window initialized successfully')

# Set up arduino serial connection
bRate = 9600 # Baudrate MUST match arduino
ports = list_ports.comports(include_links=False)
print('Available COM ports:')
for port in ports:
    print(port.device)
print('Make sure it selects the correct serial bus for the arduino!')

# Set up arduino serial object
arduino = serial.Serial(ports[0].device, bRate, timeout = 0.5)

# Set up protocol ready for start
doRun = False
print('Python-arduino communication initialized successfully\n............\nWaiting for spacebar to start/pause training\nEsc x 2 to quit protocol completely')

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
while doRun:
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
    # Get arduino data
    checkArduino()