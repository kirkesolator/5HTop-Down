#include <Arduino.h>
// Generate LED stimulus matrix according to arduino mega pin map
int STIMmatrix1[4][3] = // First stimulus pin ID array
{
  {28, 31, 23},
  {27, 31, 24},
  {26, 31, 25},
  {22, 31, 29}
};
int STIMmatrix2[4][5] = // Second stimulus pin ID array
{
  {24, 26, 28, 22, 27},
  {28, 22, 27, 25, 23},
  {27, 25, 23, 29, 24},
  {23, 29, 24, 26, 28}
};
int stimSpace = 100; // Stimulus spacing
int ISI = 100; // Inter stimulus interval

void setup()
{
  // Initialize pins for output function
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 3; j++)
    {
      pinMode(STIMmatrix1[i][j], OUTPUT);
    }
  }
}

// Main loop
void loop()
{
  // Run through the 4 first stimulus patterns
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      digitalWrite(STIMmatrix1[i][j],HIGH);
    }
    delay(stimSpace);
    for (int j = 0; j < 3; j++)
    {
      digitalWrite(STIMmatrix1[i][j],LOW);
    }
  }
  // Run through the 4 better stimulus patterns
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 5; j++)
    {
      digitalWrite(STIMmatrix2[i][j],HIGH);
    }
    delay(stimSpace);
    for (int j = 0; j < 5; j++)
    {
      digitalWrite(STIMmatrix2[i][j],LOW);
    }
  }
  delay(ISI);
}
