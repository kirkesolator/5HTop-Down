//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: OVERVIEW  :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  /*
  */

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: LIBRARY :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  #include <Arduino.h>
  #include <LiquidCrystal.h>
  #include <avr/interrupt.h>          // For random seed gen
  #include <avr/wdt.h>                // For random seed gen
  #include <util/atomic.h>            // For random seed gen
  #define randomSeed(s) srandom(s)    // For random seed gen


//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: TRAINING :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  // const int stateDepVec[7] = {1,2,3,4,5,6,0}; // Full stimulus
  const int stateDepVec[7] = {3,2,3,4,5,6,0}; // Only odors
  // const int stateDepVec[7] = {1,2,4,4,5,6,0}; // Only visual
  // const int stateDepVec[7] = {5,2,4,4,5,6,0}; // Only reward
  bool rewardEvery = false;

//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: VARIABLES :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  // ---Time variables
    const int tITI = 1000;  // Inter-trial interval (ms)
    const int tReward = 20; // Reward duration (ms)
    const int tOffsetR[2] = {200, 800}; // Random wait range for reward onset after S1 offset (ms)
    int tOffset;

  // ---Stimulus varibles
    const int tS1 = 1000;   // S1 stimulus duration (ms)
    const int tS2 = 1000;   // S2 stimulus duration (ms)
    int S2id = 0;           // S2 odor identity
    const int vDelay = 100;  // S1 onset/offset delay because of serial port
    
    const int nOdors = 4;
    int odorSel[2][nOdors] = {
      {1,2,3,4},
      {1,1,0,0}
    };
    int odorProb[4][6] = {
      { 0, 1, 1, 2, 2, 3},
      { 1, 0, 2, 1, 3, 2},
      {10,10,10,10,10,10},
      {90,90,90,90,90,90}
    };
    
    // Olfactometer
      const int nStim = 7;
      int stimPins[4][nStim]; // Empty array to map pins to laters
      int odorPins[4][nStim] = {
        {0,0,0,3,3,3,6},    // Valve 1
        {1,2,2,4,5,5,7}, // Valve 2
        {0,1,1,0,1,1,0}, // Valve 1 value
        {1,0,1,1,0,1,1}  // Valve 2 value
      };
      int stimVal[2] = {LOW,HIGH}; // Pin write values

      int nPins = sizeof(outPins)/sizeof(outPins[0]);
      const int defaultStim = 0; // All zeros will leave olfactometer in exhaust mode (no odors or clean channel)

  // ---General variables
    int outPins[10] = {22,23,24,25,26,27,28,29,2,13}; // Needs to be filled with actual output pin IDs (EXCLUDING lick detector pin)
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
    const unsigned int rateBaud = 28800; // Baud rate for serial com. !!CRITICAL that this is same in python script!!

  // ---f(timer variables)
    unsigned long pTimer;
    unsigned int tTimer;
    bool doTime = false;
    unsigned long pTimer2;
    const int tTimer2 = 40; // This sets the rate of treadmill sampling (40 = 25Hz..)

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
    unsigned int pReward[nOdors] = {100,100,0,0}; // TODO:+++ THIS IS WRONG, NEED TO USE THE ODOR ID INSTEAD
    int pinReward[1] = {8};

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

  // 6. f(map stimulus vector to output pins)___________________________________________________________________
    void mapoutpins(int outPins[],int stimPins[4][nStim],int odorPins[4][nStim],int nStim){
      for(int j = 0; j < 2; j++){
        for (int i = 0; i <= nStim; i++){
          stimPins[j][i] = outPins[odorPins[j][i]];
        }
      }
    }
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

    // Map S2 output pins onto simulus matrix
    mapoutpins(outPins,stimPins,odorPins,nStim);

    // End. Wait with next stage until serial monitor is running
    while (! Serial);
  }



//:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_: MAIN LOOP :_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:_:
  void loop() {
  //.............................. SERIAL INPUT ..............................
    if (Serial.available() > 0) {
      // Read the incoming byte
      incomingByte = Serial.read();
      Serial.flush();
      // Start/stop reward delivery via SPACE BAR
      if (incomingByte == 83){
        doRun = !doRun;
        if (doRun){
          Serial.print("\n:S:");
          Serial.println(millis()); // Time stamp
          doRun = true;
          bgtimer(0);
        }
        else if (!doRun){
          Serial.print("\n:SS:");
          Serial.println(millis()); // Time stamp
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
  
  //.............................. Treadmill ..............................
    if(millis() - pTimer2 >= tTimer2 && doRun){
        if(analogRead(pinTM) != valTM){
          valTM = analogRead(pinTM);
          pTimer2 = millis();
          Serial.print(" $"); // Trial ID event key
          Serial.print(valTM); // Trial ID
          Serial.print(":");
          Serial.print(pTimer2); // Time stamp
        }
    }
 
  //.............................. STATE PROGRESSION ..............................
    if (doRun){
      // Start block------------
      switch(state){
      case 0: 
        // 1. Start ITI timer
        bgtimer(tITI-vDelay);

        // 2. Define current trial as modulo trial types
        currentTrial = Ntrials % (nTrialsinBlock * nTrialTypes);

        // 3. Shuffle block vector to randomize upcoming trials IF new block
        if (currentTrial == 0){
          for (k = (nTrialsinBlock * nTrialTypes)-1; k >= 0; k--){
            j = random(k); // Pick a random index from 0 to k
            swap(&vecBlock[k], &vecBlock[j]); // Swap vecBlock[k] with the element at random index
          }
          Nblocks += 1; // Update block counter
          Serial.print(":X"); // Trial ID event key
          Serial.print(Nblocks); // Trial ID
          Serial.print(":");
          Serial.println(millis()); // Time stamp
        }

        // 4. Reset lick counter
        lickCount = 0;
        
        // 5. Figure out this trial's random reward time offset
        tOffset = random(tOffsetR[0],tOffsetR[1]); // Input is [min,max] as defined in tOffsetR

        // 6. Determine this trial's odor and reward outcome 
        if(random(100) > odorProb[2][vecBlock[currentTrial]]){
          S2id = odorProb[0][vecBlock[currentTrial]];
          giveReward = odorSel[1][S2id];
        }
        else{
          S2id = odorProb[1][vecBlock[currentTrial]];
          giveReward = odorSel[1][S2id];
        }
        if(rewardEvery){
          giveReward = true;
        }

        // 7. Update trial counter
        Ntrials += 1;
        Serial.print("Z"); // Trial ID event key
        Serial.print(Ntrials); // Trial ID
        Serial.print(":");
        Serial.println(millis()); // Time stamp

        // 9. Send serial data to python
          // 1. Lick data
          for (i = 0; i < lickCount; i++){
            Serial.print("€"); // Lick event key
            Serial.print(vecLick[0][i]);
            Serial.print(":"); // Lick timestamp key
            Serial.println(vecLick[1][i]);
          }
        
        // 10. Define next transition state
        transition(stateDepVec[0]);
        break;

      case 1: // Turn on S1
        bgtimer(tS1);
        transition(stateDepVec[1]);
        Serial.print("\nQ"); // Trial ID event key
        Serial.print(vecBlock[currentTrial]);
        Serial.print(':');
        Serial.println(millis()); // Time stamp
        digitalWrite(13, HIGH);
        break;

      case 2: // Turn off S1 
        transition(stateDepVec[2]);
        bgtimer(vDelay);
        Serial.print("\nW:"); // Trial ID event key
        Serial.println(millis()); // Time stamp
        digitalWrite(13, LOW);
        break;

      case 3: // Turn on S2
        // Turn on the correct combination of pins to drive odor delivery
        for (int i = 0; i < 2; i++){
          digitalWrite(stimPins[i][S2id],stimVal[stimPins[i+2][S2id]]); // 
          digitalWrite(stimPins[i][defaultStim],LOW);
        }
        digitalWrite(13,HIGH);
        bgtimer(tS2);
        transition(stateDepVec[3]);
        Serial.print("\nE"); // Trial ID event key
        Serial.print(S2id);
        Serial.print(":");
        Serial.println(millis()); // Time stamp
        break;

      case 4: // Turn off S2 and start timer for reward offset
        // Reset the stimulus ID and pin vector
        for (int i = 0; i < 2; i++){
          digitalWrite(stimPins[i][S2id],LOW);
          digitalWrite(stimPins[i][defaultStim],stimVal[stimPins[i+2][defaultStim]]);
        }
        digitalWrite(13,LOW);
        transition(stateDepVec[4]);
        bgtimer(tOffset);
        Serial.print("\nR:"); // Trial ID event key
        Serial.println(millis()); // Time stamp
        break;

      case 5: // Turn on reward
        if(giveReward){
          output(pinReward,1,HIGH);
          Serial.print("\nT:"); // Trial ID event key
        }
        else{
          output(pinReward,1,LOW);
          Serial.print("\nY:"); // Trial ID event key
        }
        bgtimer(tReward);
        Serial.println(millis()); // Time stamp
        transition(stateDepVec[5]);
        break;

      case 6: // Turn off reward
        if(giveReward){
          output(pinReward,1,LOW);
        }
        transition(stateDepVec[6]);
        bgtimer(0);
        Serial.print("\nU:"); // Trial ID event key
        Serial.println(millis()); // Time stamp
        break;

      default: // Default behaviour
        break;
      }
    }
  }