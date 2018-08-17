// -----
// DrehGeber.cpp - Library for using rotary encoders.
// This class is implemented for use with the Arduino environment.
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
// More information on: http://www.mathertel.de/Arduino
// -----
// 18.01.2014 created by Matthias Hertel
// 17.06.2015 minor updates.
// -----

#include <Arduino.h>
#include "DrehGeber.h"

// The array holds the values �1 for the entries where a position was decremented,
// a 1 for the entries where the position was incremented
// and 0 in all the other (no change or not valid) cases.
const int8_t KNOBDIR[] = {
  //	PinA			  0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
  //	PinB			  0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
  //	PinA(alt)		0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
  //	PinB(alt)		0   0   0   0   0   0   0   0   1   1   1   1	  1   1   1   1
  //	-> Richtung 
                  0, -1,  1,  0,  1,  0,  0, -1, -1,  0,  0,  1,  0,  1, -1,  0
};


// ----- Initialization and Default Values -----

DrehGeber::DrehGeber(int pinA, int pinB, int pinButton) {

  // Remember Hardware Setup
  _pinA = pinA;
  _pinB = pinB;
  _pinButton = pinButton;

  // Setup the input pins
  pinMode(pinA, INPUT_PULLUP);
//  digitalWrite(pinA, HIGH);   // turn on pullup resistor

  pinMode(pinB, INPUT_PULLUP);
//  digitalWrite(pinB, HIGH);   // turn on pullup resistor

  pinMode(pinButton, INPUT);
  //  digitalWrite(pin2, HIGH);   // turn on pullup resistor

///  // Set original state to current state
///  int sig1 = digitalRead(_pinA);
///  int sig2 = digitalRead(_pinB);
///  _oldState = sig1 | (sig2 << 1);
  _oldState = 0;

  // start with position ;
  _position = 0;

  // Set original state of button to current state
  _old_buttonState = digitalRead(_pinButton);

  // _Last_Change = Nothing
  _Last_Change = Nothing;
} // DrehGeber()


long  DrehGeber::getPosition() {
  return _position;
} // getPosition()


void DrehGeber::setPosition(long newPosition) {
  _position = newPosition;
} // setPosition()

static unsigned long C = 0;

bool DrehGeber::tick(void)
{
  byte sig1 = digitalRead(_pinA);
  byte sig2 = digitalRead(_pinB);
  byte thisState = sig1 | (sig2 << 1);

  // Das ist nicht gut (HW defekt?)
  // Immerhin kann ich eine Drehung detektieren, wenn auch nicht die Richtung
  if(thisState == 3) {
      _oldState = thisState; // "Scharf machen"
  } else if(_oldState == 3) {
//      D_PRINT(C++); D_PRINT(":");
//      D_PRINT(thisState&1);D_PRINT((thisState&2)>>1);
//      D_PRINT(_oldState&1);D_PRINT((_oldState&2)>>1);
//      D_PRINT(":");D_PRINT(KNOBDIR[thisState | (_oldState << 2)]);
//      if(thisState == 3) {
//        D_PRINTLN("NIX");
//        _oldState = thisState;
//      } else {
//        D_PRINT("Dreh");
        _position ++;
        _oldState = thisState;
        _Last_Change = Rotated_Plus;
        return true;
      }
      /*
    if (_oldState != thisState) {
      switch (KNOBDIR[thisState | (_oldState << 2)]) {
        case 1:
          D_PRINTLN("Rotate +");
          _position += KNOBDIR[thisState | (_oldState << 2)];
          _oldState = thisState;
          _Last_Change = Rotated_Plus;
          return true;
          break;
        case -1:
          D_PRINTLN("Rotate -");
          _position--;
          _oldState = thisState;
          _Last_Change = Rotated_Minus;
          return true;
          break;
        default:
          D_PRINTLN("Rotate NIX");
          _oldState = thisState;
  //        sig1 = digitalRead(_pinA);
  //        sig2 = digitalRead(_pinB);
  //        _oldState  = sig1 | (sig2 << 1);
          break;
      }
  }
*/

  sig1 = digitalRead(_pinButton);
  if ( sig1 != _old_buttonState) {
    _old_buttonState = sig1;
    if (sig1 == HIGH) {
///      D_PRINTLN("Button_Released");
      _Last_Change = Button_Released;
    } else {
///      D_PRINTLN("Button_Pressed");
      _Last_Change = Button_Pressed;
    }
    return true;
  }

  _Last_Change = Nothing;
  return false;
} // tick()

// End
