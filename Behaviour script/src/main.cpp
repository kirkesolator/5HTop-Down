#include <Arduino.h>

// Declare some parameters
const int tITI = 2000; // ITI duration
const int tS2 = 500; // Stimulus S2 duration
const int tS1 = 500; // Stimulus S1 duration
const int tReward = 20; // Reward water flow duration
const int tTrace = 500; // Trace period duration
const int tDelay = 10; // Reward onset delay

// Timer parameters
int unsigned long pTimer;
int unsigned long tTimer;
int doTime = 0;

// General parameters
int state = 0;
int nextstate = 0;
const int outpins[] = {22, 23, 24, 25, 26, 27, 28, 29, 31}; // All out pins
int LEDpins[4][5] = { // Pins that drive the various LED signals used
  {28,26,24,29,23},
  {24,29,23,25,27},
  {23,25,27,22,28},
  {27,22,28,26,24}
};
int trialState[8][3] = { // Simplified trial type joint prob matrix
  {1, 2, 85},
  {2, 1, 85},
  {3, 6, 85},
  {4, 5, 60},
  {5, 4, 60},
  {6, 3, 85},
  {7, 8, 85},
  {8, 7, 85}
};
int rPin[] = {31};
int i;
int j;
int usepins[5];

// SUB ROUTINE FUNCTIONS -----------------
// State transitions
void transition(int n)
{
  state = -1;     // Reset state to default
  nextstate = n;  // Advance state index
}

// Output pin toggle
void output(int ppins[], int n, char direction)
{
  for(int i = 0; i < n; i++)
  {
    digitalWrite(ppins[i], direction);
  }
}

// Background timer
void bgtimer(int tDur)
{
  doTime = 1;
  pTimer = millis();
  tTimer = tDur;
}


// ---------------------------------------
void setup()
{
  Serial.begin(9600);
  int nPins = sizeof(outpins)/sizeof(outpins[0]);

  // Set pinmodes to output
  for (i = 0; i < nPins+1; i++)
  {
    pinMode(outpins[i],OUTPUT);
  }
}

// ---------------------------------------
void loop()
{
  // Timer
  if (millis() - pTimer >= tTimer)
  {
    state = nextstate; // Trigger state transition via switch cases below
    doTime = 0;
  }

  // Implement state transitions
    switch (state)
    {
      case 0: // ITI timer start
        bgtimer(tITI); // Start timer
        transition(1); // Advance state index
        break;
      case 1: // S2 LED ON
        // j = random(0,3);
        j += 1;
        j = j%4;
        for (i = 0; i < 5; i++)
        {
          usepins[i] = LEDpins[j][i];
        }
        Serial.print(j);
        bgtimer(tS2); // Start timer
        output(usepins, 5, HIGH); // activate(pins);
        transition(2); // Advance state index
        break;
      case 2: // S2 LED OFF
        output(usepins, 5, LOW); // activate(pins);
        transition(3); // Advance state index
        break;
      case 3: // Trace period start
        bgtimer(tTrace); // Start timer
        transition(4); // Advance state index
        break;
      case 4: // S1 odor  ON
        output(rPin, 1, HIGH);
        bgtimer(tS1); // Start timer
        transition(5); // Advance state index
        break;
      case 5: // S1 odor presentation OFF
        output(rPin, 1, LOW); //S1 odor OFF
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
      case 8: // Reward presentation ON
        output(rPin, 1, LOW); // Close reward port
        transition(0); // Advance state index
        break;
      default: // Default stay out of switch
        break;
    }
}
