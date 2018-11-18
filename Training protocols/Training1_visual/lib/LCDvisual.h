/*
  Morse.h - Library for flashing Morse code.
  Created by David A. Mellis, November 2, 2007.
  Released into the public domain.
*/
#ifndef Vstim_h
#define Vstim_h

#include "Arduino.h"

class Vstim
{
  public:
    Vstim(int pin);
    void start();
    void stop();
  private:
    int _stimID;
};

#endif