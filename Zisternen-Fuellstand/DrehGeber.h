// -----
// DrehGeber.h - Basierend auf RotaryEncoder von Matthias Hertel
// -----

#ifndef DrehGeber_h
#define DrehGeber_h

#include "Arduino.h"

class DrehGeber
{
  public:
    // ----- Constructor -----
    DrehGeber(int pin1, int pin2, int pin3);

    typedef enum Change_e {
      Nothing = 0,
      Button_Pressed,
      Button_Released,
      Rotated_Plus,
	  Rotated_Minus
    } Change;

    // call this function every some milliseconds to detect a state changes of the rotary encoder.
    bool tick(void);

    // when tick returns true, find out what has changed
    Change getChange() {
      return _Last_Change;
    };

    // retrieve the current position
    long  getPosition();

    // adjust the current position
    void setPosition(long newPosition);

    // retrieve Button state
    bool getButton() {
      return _old_buttonState;
    };

  private:
    int _pinA, _pinB; 		// Arduino pins used for the encoder.
    int _pinButton;			// Arduino pin used for button

    int8_t _oldState;			// Combination of (old) PinA + PinB<<1;
    long _position;     		// Counter of position

    int _old_buttonState;

    Change _Last_Change;		// When a change was detected, contains what type it was

};

#endif

// End
