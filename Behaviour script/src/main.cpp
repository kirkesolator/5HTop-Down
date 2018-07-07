#include <Arduino.h>

// -----Duration parameters-----
const int tITI = 2000; // ITI duration
const int tS1 = 500; // Stimulus S2 duration
const int tS2 = 500; // Stimulus S1 duration
const int tReward = 20; // Reward water flow duration
const int tTrace = 500; // Trace period duration
const int tDelay = 10; // Reward onset delay

// -----Timer parameters-----
unsigned long pTimer;
unsigned long tTimer;
bool doTime = false;

// -----Input parameters-----
int lickState = 1; // Initial lick state
int lickStateP; // Previous lick state
int lickNum; // Poke counter
int lickON;
int lickOFF;
unsigned long treadState;

// -----ID variables-----
int mouseID;
String humanID;

// -----Pin parameters-----
const unsigned int outPins[21] = {1,2,3,4,5,6,7,8,9,10,11,12,22,23,24,25,26,27,28,29,31}; // All out pins
const unsigned int ledPins[4][5] = { // Pins that drive the various LED signals used
  {28,26,24,29,23},
  {24,29,23,25,27},
  {23,25,27,22,28},
  {27,22,28,26,24}
};
const unsigned int trialStates[8][3] = { // Simplified trial type joint prob matrix
  {1, 2, 85},
  {2, 1, 85},
  {3, 6, 85},
  {4, 5, 60},
  {5, 4, 60},
  {6, 3, 85},
  {7, 8, 85},
  {8, 7, 85}
};
const unsigned int odorPins[4][12] = { // Olfactometer pin mapping
  {1,1,1,4,4,4,7,7,7,10,10,10},
  {2,3,3,5,6,6,8,9,9,11,12,12},
  {0,1,1,0,1,1,0,1,1,0,1,1},
  {1,0,1,1,0,1,1,0,1,1,0,1}
};
int rPin[] = {31}; // Reward valve pin
int lPin = 40; // Lick pin
int treadPin = A0;
int S1pins[5];
int S2pins[2];
int unsigned S1ID;

// -----General parameters-----
int blockBasis[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int blockSize = sizeof(blockBasis)/sizeof(blockBasis[0]);
unsigned int inblock = false;
unsigned int blockPos = 0;
int unsigned trialCoin;
int unsigned trialID;
// Variables
int state = 0;
int nextstate = 0;
int i;
int j;


//:::::::::::::::::::::::::---------:::::::::::::::::::::::::
//:::::::::::::::::::::::::FUNCTIONS:::::::::::::::::::::::::
// f(state transition)-----
void transition(int n)
{
  state = -1;     // Reset state to default
  nextstate = n;  // Advance state index
}

// f(output pin toggle)-----
void output(int ppins[], int n, char direction)
{
  for(int i = 0; i < n; i++)
  {
    digitalWrite(ppins[i], direction);
  }
}

// f(background timer)-----
void bgtimer(int tDur)
{
  doTime = true;
  pTimer = millis();
  tTimer = tDur;
}

// f(actually random seed generator function)-----
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>
// The following addresses a problem in version 1.0.5 and earlier of the Arduino IDE
// that prevents randomSeed from working properly.
//        https://github.com/arduino/Arduino/issues/575
#define randomSeed(s) srandom(s)

volatile uint32_t seed;  // These two variables can be reused in your program after the
volatile int8_t nrot;    // function CreateTrulyRandomSeed()executes in the setup()
                         // function.
void CreateTrulyRandomSeed()
{
  seed = 0;
  nrot = 32; // Must be at least 4, but more increased the uniformity of the produced
             // seeds entropy.
  // The following five lines of code turn on the watch dog timer interrupt to create
  // the seed value
  cli();
  MCUSR = 0;
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (1<<WDE);
  _WD_CONTROL_REG = (1<<WDIE);
  sei();
  while (nrot > 0);  // wait here until seed is created
  // The following five lines turn off the watch dog timer interrupt
  cli();
  MCUSR = 0;
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (0<<WDE);
  _WD_CONTROL_REG = (0<< WDIE);
  sei();
}
ISR(WDT_vect)
{
  nrot--;
  seed = seed << 8;
  seed = seed ^ TCNT1L;
}

// f(shuffle trial identities)-----
void swap (int *p,int *q)
{
  int temp;
  temp=*p;
  *p=*q;
  *q=temp;
}

//:::::::::::::::::::::::::-----:::::::::::::::::::::::::
//:::::::::::::::::::::::::-----:::::::::::::::::::::::::
//:::::::::::::::::::::::::SETUP:::::::::::::::::::::::::
void setup()
{
  CreateTrulyRandomSeed(); // Calls the true random seed generator from above
  randomSeed(seed);

  // Open serial port
  Serial.begin(9600);
  int nPins = sizeof(outPins)/sizeof(outPins[0]);

  // Set pinmodes to output
  for (i = 0; i <= nPins; i++)
  {
    pinMode(outPins[i],OUTPUT);
  }
  // Wait with next stage until serial monitor is running
  while (! Serial);

  // Verify
  Serial.print("\n Mouse ID: ");
  while (Serial.available() == 0) {}
  mouseID = Serial.parseInt();
  Serial.print("\n Human ID: ");
  while (Serial.available() == 0) {}
  humanID = Serial.readString();

  // Print some random info, literally
  Serial.print("\n Random seed generated: ");
  Serial.println(seed);
  Serial.println("Block start");
}



//:::::::::::::::::::::::::---------:::::::::::::::::::::::::
//:::::::::::::::::::::::::---------:::::::::::::::::::::::::
//:::::::::::::::::::::::::---------:::::::::::::::::::::::::
//:::::::::::::::::::::::::MAIN LOOP:::::::::::::::::::::::::
void loop()
{
  // Shuffle block vector to randomize trials
  for (int k = blockSize-1; k >= 0; k--)
  {
    int j = random(k); // Pick a random index from 0 to k
    swap(&blockBasis[k], &blockBasis[j]); // Swap blockBasis[k] with the element at random index
  }
  blockPos = 1; // Reset trial counter
  inblock = true; // Declare we are in block

  // Run block
  while (inblock)
  {
    // Cycle through all the trials in this block
    for(int trials = 0; trials < blockSize; trials++)
    {
      trialID = blockBasis[trials];
      // ..........Timer..........
      if (millis() - pTimer >= tTimer)
      {
        state = nextstate; // Trigger state transition via switch cases below
        doTime = false; // Reset timer
      }

      // ..........Lick detection..........
      lickStateP = lickState; // store previous lick state
      lickState = digitalRead(lPin); // record new lick state
      if (lickState-lickStateP < 0)
      {
        lickNum += 1; // Update lick counter
        Serial.print("."); // Print to serial monitor
        lickON = millis(); // Record lick entry
      }
      else if (lickState-lickStateP > 0) // Record lick exit
      {
        lickOFF = millis();
      }

      // Read analogue treadmill input
      treadState = analogRead(treadPin);

    // ******************** State transitions ********************
      switch (state)
      {
        case 0: // ITI timer start
          bgtimer(tITI); // Start timer
          transition(1); // Advance state index

          // Determine S1 for this trial
          trialCoin = random(100);
          if (trialCoin >= trialStates[trials][2])
          {
            S1ID = trialStates[trials][1];
          }
          else
          {
            S1ID = trialStates[trials][0];
          }
          // Generate pin map for LED stimulus
          for (i = 0; i < 5; i++)
          {
            S1pins[i] = ledPins[j][i];
          }
          // Determine S2 for this trial
          S2pins[0] = 2;
          break;
        case 1: // S1 stimulus ON
          bgtimer(tS1); // Start timer
          output(S1pins, 5, HIGH); // activate(pins);
          transition(2); // Advance state index
          break;
        case 2: // S1 stimulus OFF
          output(S1pins, 5, LOW); // activate(pins);
          transition(3); // Advance state index
          break;
        case 3: // Trace period start
          bgtimer(tTrace); // Start timer
          transition(4); // Advance state index
          break;
        case 4: // S2 odor  ON
          output(S2pins, 1, HIGH);
          bgtimer(tS2); // Start timer
          transition(5); // Advance state index
          break;
        case 5: // S2 odor presentation OFF
          output(S2pins, 1, LOW); //S1 odor OFF
          transition(6); // Advance state index
          break;
        case 6: // Reward onset delay timer start
          bgtimer(tDelay); // Start timer
          transition(7); // Advance state index
          break;
        case 7: // Reward presentation ON
          output(rPin, 1, HIGH); // Open reward port
          bgtimer(tReward); // Start timer
          transition(8); // Advance state index
          break;
        case 8: // Reward presentation OFF
          output(rPin, 1, LOW); // Close reward port
          transition(0); // Advance state index
          if (blockPos == blockSize)
          {
            inblock = false;
          }
          else blockPos += 1;
          break;
        case 9: // Pause the whole thing
          while (Serial.available() == 0) {} // Wait for input over serial port
          transition(nextstate); // Reset state index
          break;
        default: // Default stay out of switches
          break;
      }
    }
  }
}
