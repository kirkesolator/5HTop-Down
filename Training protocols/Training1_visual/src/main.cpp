//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: OVERVIEW :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
// 1. TODO: Define trial types
// --- For visual only, only 6 stimuli, BUT if we want some play on statistics, should perhaps include in this
// 2. TODO: Determine block structure
// --- Should have several rounds of the trial types   - x3? so blocks of 18 trials?
// --- Do we want to have a vector that is shuffled to generate random trial type progression?
// 3. Set up communication with PYTHON
// 4. TODO: Write a PYTHON script to save (and plot) progress
// 5. Set up RANDOM function
// 6. TODO: Set up (multi) timer
// 7. TODO: Set up STATE progression
// 8. TODO: Remember to put in a section (ITI) for sending and receiving data
// 9. TODO: Add visual stimulus function
// 10. TODO: Set up random wait time between stimulus offset and reward delivery


//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: LIBRARY :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <avr/interrupt.h> // For randdom seed gen
#include <avr/wdt.h> // For randdom seed gen
#include <util/atomic.h> // For randdom seed gen
#define randomSeed(s) srandom(s) // For randdom seed gen



//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: VARIABLES :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
// General variables
const unsigned int rateBaud = 9600;
int outPins[1]; // Needs to be filled with actual output pin IDs

// Time variables
const int tITI = 1000;  // Inter-trial interval (ms)
const int tS1 = 200;    // S1 stimulus duration (ms)
const int tReward = 20; // Reward duration (ms)
const int tRewardWait[2] = {200, 800}; // Random wait range for reward onset after S1 offset (ms)


// Trial varibles
int i;
const int nTrialTypes = 6;
const int nTrialsinBlock = 3;
int vecBlock[nTrialsinBlock*nTrialTypes]; // Cf setup 2. for generating actual vector

// Random generator variables
volatile uint32_t seed;  // These two variables can be reused in your program after the
volatile int8_t nrot;    // function CreateTrulyRandomSeed()executes in the setup()



//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: FUNCTIONS :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
// 1. f(random_generator)____________________________________________________________________________________
void trueRandSeed(){
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
// Interrupt service that goes with random generator above
ISR(WDT_vect){
  nrot--;
  seed = seed << 8;
  seed = seed ^ TCNT1L;
}

// Use in setup section 3.:
//  trueRandSeed(); // Calls the true random seed generator from above
//  randomSeed(seed);
// Use in any section:
//  // Then just call random() as usual
// -------------------------------------------------------------------------------------------------------------




//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: SETUP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
void setup() {
    // 1. Open serial com
    Serial.begin(rateBaud);

    // 2. Block vector generator
    for (i = 0; i < (nTrialsinBlock * nTrialTypes); i++){
        vecBlock[i] = i % nTrialTypes;
    }
    
    // 3. Create better random generator seed
    trueRandSeed();     // Calls the true random seed generator from above
    randomSeed(seed);   // Generate seed

    // 4. Set output pins
    int nPins = sizeof(outPins)/sizeof(outPins[0]);
    for (i = 0; i <= nPins; i++){
        pinMode(outPins[i],OUTPUT);
    }

    // Wait with next stage until serial monitor is running
    while (! Serial);
}



//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: MAIN LOOP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
void loop() {
  // Start block, define block vector

  // Define reward outcomes for any trials with uncertainty

  // Check timer

  // Check lick status

  // Send info to python

  // Check for input from key press or python

  // Output/state progression
}
