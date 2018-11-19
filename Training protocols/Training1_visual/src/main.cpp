//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: OVERVIEW  :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  /*
  1. Define trial types
  --- For visual only, only 6 stimuli, BUT if we want some play on statistics, should perhaps include in this
  2. Determine block structure
  --- Should have several rounds of the trial types   - x3? so blocks of 18 trials?
  --- Do we want to have a vector that is shuffled to generate random trial type progression?
  3. Set up communication with PYTHON
  TODO: 4. Write a PYTHON script to save (and PLOT) progress
  5. Set up RANDOM function
  6. Set up (multi) timer
  7. Set up STATE progression
  TODO: 8. Remember to put in a section (ITI) for sending and receiving data
  9. Add visual stimulus function
  10. Set up random wait time between stimulus offset and reward delivery (use map function)
  11. Key stroke input for start stop
  12. Add interrupt for lick detector
  13. You (don't) need to deal with millis rollover!!! 
    FIXME: https://arduino.stackexchange.com/questions/22994/resetting-millis-and-micros
  */

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: LIBRARY   :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
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
    int outPins[13]; // Needs to be filled with actual output pin IDs (EXCLUDING lick detector pin)
    bool doRun = false;
    int state = 0;
    int nextstate = 0;
    bool giveReward; 

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
    const int tReward = 250; // Reward duration (ms)
    const int tOffsetR[2] = {200, 800}; // Random wait range for reward onset after S1 offset (ms)
    int tOffset;

  // ---f(timer variables)
    unsigned long pTimer;
    unsigned long tTimer;
    bool doTime = false;

  // ---Trial varibles
    const int nTrialTypes = 6;
    const int nTrialsinBlock = 3;
    int vecBlock[nTrialsinBlock*nTrialTypes]; // Cf setup 2. for generating actual vector
    int Ntrials = 0;
    int Nblocks = 0;
    unsigned int currentTrial = 0;

    // ---Treadmill
    const int pinTM = A0;
    int valTM = analogRead(pinTM);

    // ---Reward variables
    unsigned int pReward[nTrialTypes] = {100,100,50,50,0,0};
    int pinReward[1] = {13};

  // ---Random generator variables
    volatile uint32_t seed;  // These two variables can be reused in your program after the
    volatile int8_t nrot;    // function truleRandSeed() executes in the setup()

  // ---Initialize general use stuff
    int i;
    int k;
    int j;

  // ---LCD visual stimulus variables
    // Define custom character shapes
    byte full[8] = { // Full
      B11111,B11111,
      B11111,B11111,
      B11111,B11111,
      B11111,B11111,
    };
    // Stimulus-cursor matrix 
    int cursorVec[2][6] = {
      {0,1, 0,1,0, 1},
      {0,5,10,0,5,10}
    };

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
    /*
    Use in setup section 3.:
      trueRandSeed(); // Calls the true random seed generator from above
      randomSeed(seed);
    Use in any section:
      Just call random() as usual
    */
  // -----------------------------------------------------------------------------------------------------------

  // 2. f(shuffle trial identities)_____________________________________________________________________________
    void swap (int *p,int *q){
    int temp;
    temp=*p;
    *p=*q;
    *q=temp;
    }
    /*
    Call with:
    for (int k = (nTrialsinBlock * nTrialTypes)-1; k >= 0; k--){
     int j = random(k); // Pick a random index from 0 to k
     swap(&vecBlock[k], &vecBlock[j]); // Swap vecBlock[k] with the element at random index
    }
    */
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

  // 4. f(state transition)_____________________________________________________________________________________
    void transition(int n){
      state = -1;     // Reset state to default
      nextstate = n;  // Advance state index
    }
    /*
    Needs decleared:
    int nextstate = 0;
    int state = 0;
    */
  // -----------------------------------------------------------------------------------------------------------

  // 5. f(output pin toggle)____________________________________________________________________________________
    void output(int ppins[], int n, char direction){
      for(int i = 0; i < n; i++){
        digitalWrite(ppins[i], direction);
      }
    }
    /*
    int pinReward[1] = {2}; // To allow vactor with only 1 element to pass to function
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

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: SETUP     :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
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
    pinMode(interruptPin, INPUT_PULLUP); //TODO: check if pullup needed
    attachInterrupt(digitalPinToInterrupt(interruptPin), recLick, CHANGE);

    // 6. Start LCD and create custom character
    lcd.begin(16, 2);
    lcd.createChar(0, full);

    // End. Wait with next stage until serial monitor is running
    while (! Serial);
  }



//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: MAIN LOOP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  void loop() {
  //.............................. SERIAL INPUT ..............................
    if (Serial.available() > 0) {
      // Read the incoming byte
      incomingByte = Serial.read();

      // Start/stop reward delivery via SPACE BAR
      if (incomingByte == 32){
        doRun = !doRun;
        if (doRun){
          Serial.println("Training protocol started");
          doRun = true;
          bgtimer(0);
        }
        else{
          Serial.println("Training protocol stopped");
          doRun = false;
          doTime = false;
        }
      }
    }
  
  //.............................. Timer ..............................
    if(doTime){
      if (millis() - pTimer >= tTimer){
          state = nextstate; // Trigger state transition via switch cases below
          doTime = false; // Reset timer
      }
    }
  
  //TODO:.............................. Treadmill ..............................
    if(analogRead(pinTM) != valTM){
      valTM = analogRead(pinTM);
      // Record time
    }

  //.............................. STATE PROGRESSION ..............................
    if (doRun){
      // Start block------------
      switch(state){
      case 0: 
        // 1. Start ITI timer
        bgtimer(tITI);

        // 2. Define current trial as modulo trial types
        currentTrial = Ntrials % (nTrialsinBlock * nTrialTypes);

        // 3. Shuffle block vector to randomize upcoming trials IF new block
        if (currentTrial == 0){
          for (k = (nTrialsinBlock * nTrialTypes)-1; k >= 0; k--){
            j = random(k); // Pick a random index from 0 to k
            swap(&vecBlock[k], &vecBlock[j]); // Swap vecBlock[k] with the element at random index
          }
          Nblocks += 1; // Update block counter
          Serial.print("Bn"); // Trial ID event key
          Serial.print(Nblocks); // Trial ID
          Serial.print(":");
          Serial.print(millis()); // Time stamp
        }

        // 4. Reset lick counter
        lickCount = 0;
        
        // 5. Figure out this trial's random reward time offset
        tOffset = random(tOffsetR[0],tOffsetR[1]); // Input is [min,max] as defined in tOffsetR

        // 6. Determine this trial's reward outcome 
        if(random(99) < pReward[vecBlock[currentTrial]]){
          giveReward = true;
        }
        else{
          giveReward = false;
        }

        // 7. Update trial counter
        Ntrials += 1;
        Serial.print("_:_Tn"); // Trial ID event key
        Serial.print(Ntrials); // Trial ID
        Serial.print(":");
        Serial.print(millis()); // Time stamp

        /* ONLY an option to use LCD for this if NOT using LCD for visual stimulation.....
        // 8. Write to LCD: Set cursor and clear LCD
        lcd.clear();
        lcd.print("Block: "); // Need block counter
        lcd.println(Nblocks);
        lcd.print("Trial: "); // Need trial counter
        lcd.print(Nblocks);
        */

        // 9. Send serial data to python
          // 1. Lick data
          for (i = 0; i < lickCount; i++){
            Serial.print("_Ld"); // Lick event key
            Serial.print(vecLick[0][i]);
            Serial.print(":"); // Lick timestamp key
            Serial.print(vecLick[1][i]);
          }
          // 2. State progression data
          // 3. Treadmill
        
        // 10. Define next transition state
        transition(1);
        break;
      case 1: // Turn on S1
        lcd.setCursor(cursorVec[1][vecBlock[currentTrial]],cursorVec[0][vecBlock[currentTrial]]);
        for(int i = 0; i < 6; i++){
          lcd.write(byte(0));
        }
        lcd.setCursor(cursorVec[1][vecBlock[currentTrial]]+2,!cursorVec[0][vecBlock[currentTrial]]);
        lcd.write(byte(0));
        lcd.write(byte(0));
        bgtimer(tS1);
        transition(2);
        Serial.print("_S1y:"); // Trial ID event key
        Serial.print(millis()); // Time stamp
        break;
      case 2: // Turn off S1
        lcd.clear();
        transition(3);
        bgtimer(tOffset);
        Serial.print("_S1n:"); // Trial ID event key
        Serial.print(millis()); // Time stamp
        break;
      case 3: // Turn on reward
        if(giveReward){
          output(pinReward,1,HIGH);
          Serial.print("_Ryy:"); // Trial ID event key
        }
        else{
          Serial.print("_Ryn:"); // Trial ID event key
        }
        bgtimer(tReward);
        Serial.print(millis()); // Time stamp
        transition(4);
        break;
      case 4: // Turn off reward
        if(giveReward){
          output(pinReward,1,LOW);
        }
        transition(0);
        bgtimer(0);
        Serial.print("_Rn:"); // Trial ID event key
        Serial.print(millis()); // Time stamp
        break;
      default: // Default behaviour
        break;
      }
    }
  }

