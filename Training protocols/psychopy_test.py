from psychopy import visual, core
import numpy as np
import datetime as dt
import serial, os
from serial.tools import list_ports
import msvcrt

if 1:
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

# Set up arduino serial connection
bRate = 9600 # MUST match arduino
ports = list_ports.comports(include_links=False)

print('Available COM ports:')
for port in ports:
    print(port.device)
print('Make sure it selects the correct serial bus for the arduino!')

# Set up arduino serial object
arduino = serial.Serial(ports[0].device, bRate, timeout = 0.5)

print('Arduino set up in python script, waiting for keypress= s to start training')

doRun = False
while not doRun:
    if msvcrt.kbhit():
            keyMe = msvcrt.getch()
            
            if keyMe == 's':
                doRun = True
                print "Starting behavioural protocol"
                break

# for i in range(1):
#     print(arduino.readline())

if doRun:
    # Create visual stimulation window
    win=visual.Window(size = (1200,800),winType = 'pyglet',screen = 1,fullscr = True, units = 'height')

    # Generate Gabor stimulus
    gabor = visual.GratingStim(win, tex='sin', mask='gauss', sf=6, name='gabor',size=0.35)
    gabor.setAutoDraw(True)  # Automatically draw every frame
    gabor.autoLog=False # Turn off messages about phase changes (or it will go crazy)
    test = []
    # Run stimulus presentation
    for useOri in (0,60,120,180,240,300): # 6 equidistant angles
        gabor.pos = (np.sin(np.pi*useOri/180)*.35, np.cos(np.pi*useOri/180)*.35)
        gabor.ori = useOri + 90 # Grating orthogonal to positional angle
        gabor.contrast = 0 # Initially gray
        clock = core.Clock() # Start timer
        win.flip() # Flip image onto screen
        while clock.getTime() < 1.5:
            if .5 < clock.getTime() < 1.5:
                gabor.contrast = 1
                gabor.setPhase(0.08, '+')
                win.flip()
                # test.append(arduino.readline())
            if msvcrt.kbhit():
                keyMe = msvcrt.getch()
                
                if keyMe == 's':
                    doRun = False
                    print "Stopping behavioural protocol"
                    break

f.close()
os.chdir(path)
arduino.close()