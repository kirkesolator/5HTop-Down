//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: OVERVIEW :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  // 1. Define trial types
  // --- For visual only, only 6 stimuli, BUT if we want some play on statistics, should perhaps include in this
  // 2. Determine block structure
  // --- Should have several rounds of the trial types   - x3? so blocks of 18 trials?
  // --- Do we want to have a vector that is shuffled to generate random trial type progression?
  // 3. Set up communication with PYTHON
  //TODO: 4. Write a PYTHON script to save (and plot) progress
  // 5. Set up RANDOM function
  //TODO: 6. Set up (multi) timer
  //TODO: 7. Set up STATE progression
  //TODO: 8. Remember to put in a section (ITI) for sending and receiving data
  //TODO: 9. Add visual stimulus function
  //TODO: 10. Set up random wait time between stimulus offset and reward delivery (use map function)
  //TODO: 11. Key stroke input for start stop
  //TODO: 12. Add interrupt for lick detector
  //TODO: 13. You need to deal with millis rollover!!! FIXME: https://www.baldengineer.com/arduino-how-do-you-reset-millis.html


//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: LIBRARY :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  #include <Arduino.h>
  #include <LiquidCrystal.h>
  #include <avr/interrupt.h>          // For random seed gen
  #include <avr/wdt.h>                // For random seed gen
  #include <util/atomic.h>            // For random seed gen
  #define randomSeed(s) srandom(s)    // For random seed gen

  // ---Initialize LCD object
    LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: VARIABLES :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  // ---General variables
    int outPins[1]; // Needs to be filled with actual output pin IDs (EXCLUDING lick detector pin)
    bool doRun = false;
    int state = 0;

  // ---Lick detector variables
    const int interruptPin = 2; // This lick detector input MUST be either of {2,3,18,19,20,21} for interrupt to work properly
    int lickState = 0;
    volatile long vecLick[2][30];
    volatile int lickCount = 0;

  // ---Serial communication
    int incomingByte = 0;
    const unsigned int rateBaud = 9600; // Baud rate for serial com. !!CRITICAL that this is same in python script!!

  // ---Time variables
    const int tITI = 1000;  // Inter-trial interval (ms)
    const int tS1 = 200;    // S1 stimulus duration (ms)
    const int tReward = 20; // Reward duration (ms)
    const int tRewardWait[2] = {200, 800}; // Random wait range for reward onset after S1 offset (ms)

  // ---f(timer variables)
    unsigned long pTimer;
    unsigned long tTimer;
    bool doTime = false;

  // ---Trial varibles
    const int nTrialTypes = 6;
    const int nTrialsinBlock = 3;
    int vecBlock[nTrialsinBlock*nTrialTypes]; // Cf setup 2. for generating actual vector

  // ---Random generator variables
    volatile uint32_t seed;  // These two variables can be reused in your program after the
    volatile int8_t nrot;    // function truleRandSeed() executes in the setup()

  // ---Initialize general use stuff
    int i;
    int k;
    int j;

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
  // -----------------------------------------------------------------------------------------------------------

  // 2. f(shuffle trial identities)_____________________________________________________________________________
    void swap (int *p,int *q){
    int temp;
    temp=*p;
    *p=*q;
    *q=temp;
    }
    // Call with:
    // for (int k = (nTrialsinBlock * nTrialTypes)-1; k >= 0; k--){
    //  int j = random(k); // Pick a random index from 0 to k
    //  swap(&vecBlock[k], &vecBlock[j]); // Swap vecBlock[k] with the element at random index
    // }
  // -----------------------------------------------------------------------------------------------------------

  // 3. f(background timer)_____________________________________________________________________________________
    void bgtimer(int tDur){
    doTime = true;
    pTimer = millis();
    tTimer = tDur;
    }
    /*
    // -----Timer parameters-----
    unsigned long pTimer;
    unsigned long tTimer;
    bool doTime = false;
    */
  // -----------------------------------------------------------------------------------------------------------

  //:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: ISR :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  // Interrupt Service Routine for lick detection_______________________________________________________________
    void recLick(){
    lickState = digitalRead(interruptPin);
    lickCount += 1;
    vecLick[0][lickCount] = lickState;
    vecLick[1][lickCount] = millis();
    }
  // -----------------------------------------------------------------------------------------------------------

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

    // 5. Set up interrupt for lick detector
    attachInterrupt(digitalPinToInterrupt(interruptPin), recLick, CHANGE);

    // End. Wait with next stage until serial monitor is running
    while (! Serial);
  }



//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: MAIN LOOP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  void loop() {
    // ........Serial communication..........
    if (Serial.available() > 0) {
      // Read the incoming byte
      incomingByte = Serial.read();

      // Start/stop reward delivery via SPACE BAR
      if (incomingByte == 32){
        doRun = !doRun;
        if (doRun){
          Serial.println("Training protocol started");
          doRun = true;
        }
        else{
          Serial.println("Training protocol stopped");
          doRun = false;
        }
      }
    }

    if (doRun){
      // Start block------------
      switch(state){
      
      case 0: 
        // 1. Shuffle block vector to randomize upcoming trials
        for (k = (nTrialsinBlock * nTrialTypes)-1; k >= 0; k--){
          j = random(k); // Pick a random index from 0 to k
          swap(&vecBlock[k], &vecBlock[j]); // Swap vecBlock[k] with the element at random index
        }

        // Send lick data over serial com to python
        for (i = 0; i < lickCount; i++){
          Serial.print("Ld"); // Lick event key
          Serial.print(vecLick[0][i]);
          Serial.print("Lt"); // Lick timestamp key
          Serial.print(vecLick[1][i]);
        }
        // Reset lick counter
        lickCount = 0;
        break;
      case 1:

      // Define reward outcomes for any trials with uncertainty

      // Check timer

      // Check lick status

      // Send info to python

      // Check for input from key press or python

      // Output/state progression
      default:
      break;
      }
    }
  }
/* Overview
  case 0: ITI: set time ref first, then block vector generation, random offset, serial port check, 
  serial data sender, write to LCD, reset lick counter then finally run timer
  case 1: S1 onset
  case 2: S1 offset
  case 3: Timer with random offset
  case 4: Reward onset
  case 5: Reward offset
*/
