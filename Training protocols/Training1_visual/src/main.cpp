//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: OVERVIEW :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
// 1. Define trial types
// --- For visual only, only 6 stimuli, BUT if we want some play on statistics, should perhaps include in this
// 2. Determine block structure
// --- Should have several rounds of the trial types   - x3? so blocks of 18 trials?
// --- Do we want to have a vector that is shuffled to generate random trial type progression?
// 3. Set up communication with PYTHON
// 4. Write a PYTHON script to save (and plot) progress
// 5. Set up RANDOM function
// 6. Set up (multi) timer
// 7. Set up STATE progression
// 8. Remember to put in a section (ITI) for sending and receiving data
// 9. Add visual stimulus function


//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: LIBRARY :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
#include <Arduino.h>

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: VARIABLES :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
const int nTrials = 6;


//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: FUNCTIONS :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: SETUP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
void setup() {
  
}

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: MAIN LOOP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
void loop() {
  
}
