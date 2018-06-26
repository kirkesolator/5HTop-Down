#include <Arduino.h>

// -----Duration parameters-----
const int tITI = 2000; // ITI duration
const int tS2 = 500; // Stimulus S2 duration
const int tS1 = 500; // Stimulus S1 duration
const int tReward = 20; // Reward water flow duration
const int tTrace = 500; // Trace period duration
const int tDelay = 10; // Reward onset delay

// -----Timer parameters-----
int unsigned long pTimer;
int unsigned long tTimer;
bool doTime = false;

// -----ID variables-----
int mouseID;
String humanID;

// -----Pin parameters-----
const int outPins[] = {22, 23, 24, 25, 26, 27, 28, 29, 31}; // All out pins
int ledPins[4][5] = { // Pins that drive the various LED signals used
  {28,26,24,29,23},
  {24,29,23,25,27},
  {23,25,27,22,28},
  {27,22,28,26,24}
};
int trialStates[8][3] = { // Simplified trial type joint prob matrix
  {1, 2, 85},
  {2, 1, 85},
  {3, 6, 85},
  {4, 5, 60},
  {5, 4, 60},
  {6, 3, 85},
  {7, 8, 85},
  {8, 7, 85}
};
int rPin[] = {31}; // Reward valve pin
int usePins[5]; // Initialise

// -----General parameters-----
int state = 0;
int nextstate = 0;
int i;
int j;



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




//:::::::::::::::::::::::::SETUP:::::::::::::::::::::::::
void setup()
{
  CreateTrulyRandomSeed(); // Calls the true random seed generator from above
  randomSeed(seed);

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

  Serial.print("\n Random seed generated: ");
  Serial.println(seed);
  Serial.println("Block start");
}






//:::::::::::::::::::::::::MAIN LOOP:::::::::::::::::::::::::
void loop()
{
  // ..........Timer..........
  if (millis() - pTimer >= tTimer)
  {
    state = nextstate; // Trigger state transition via switch cases below
    doTime = false; // Reset timer
  }

  // ..........State transitions..........
    switch (state)
    {
      case 0: // ITI timer start
        bgtimer(tITI); // Start timer
        transition(1); // Advance state index
        break;
      case 1: // S2 LED ON

          //:::::::::::::::::::::::::
          // This is just to test LED stimuli
          // j = random(0,3);
          j += 1;
          j = j%4;
          for (i = 0; i < 5; i++)
          {
            usePins[i] = ledPins[j][i];
          }
          Serial.print(j);
          //:::::::::::::::::::::::::

        bgtimer(tS2); // Start timer
        output(usePins, 5, HIGH); // activate(pins);
        transition(2); // Advance state index
        break;
      case 2: // S2 LED OFF
        output(usePins, 5, LOW); // activate(pins);
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
