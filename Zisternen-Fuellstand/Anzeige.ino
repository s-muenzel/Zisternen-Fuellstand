#include <LiquidCrystal_I2C.h>
#include "Anzeige.h"

LiquidCrystal_I2C Lcd(0x27, 16, 2);		// I2C Adresse 0x27, 16 Zeichen, 2 Zeilen

Anzeige::Anzeige() {
  _Timeout_Licht = 0;					// kein Licht An
  _Blinken_Licht_An = 0;				// Wert 0 == kein Blinken
  _Blinken_Licht_Aus = 0;

  _Licht_ist_an = false;

  _Liter = 0;
  _Prozent = 0;
  _Min = 0;
  _Akt = 0;
  _Max = 0;
  _Verbrauch = 0;

  _Modus = Verbrauch;

  _Text_Update = false;
}

void Anzeige::begin() {
  Lcd.begin();
}

void Anzeige::tick() {
  // Text
  if (	_Text_Update) {
    Lcd.clear();
    Lcd.setCursor(0, 0);
    Lcd.print("Rest:");
    Lcd.print(_Liter);
    Lcd.print("l");
    Lcd.setCursor(12, 0);
    Lcd.print(_Prozent);
    Lcd.print("%");
    switch (_Modus) {
      case Verbrauch:
        Lcd.setCursor(0, 1);
        Lcd.print("Total: ");
        Lcd.print(_Verbrauch);
        Lcd.print(" l");
        break;
      case MinMax:
        Lcd.setCursor(0, 1);
        Lcd.print(_Min);
        Lcd.print("<");
        Lcd.print(_Akt);
        Lcd.print("<");
        Lcd.print(_Max);
        break;
      case Reset_MinMax:
        Lcd.setCursor(0, 1);
        Lcd.print("Reset Min/Max ?");
        break;
    }
    _Text_Update = false;
  }
  //Licht
  if ( _Timeout_Licht > 0) {
	  unsigned long Jetzt = millis();
	  if (_Licht_ist_an) {
		if (Jetzt > _Timeout_Licht) { // Licht ist an und muss ausgeschaltet werden
			D_PRINTLN("Licht Aus");
			_Licht_ist_an = false;
			Lcd.noBacklight();
			_Modus = Verbrauch; // zurueck in Grundzustand (Im Blink-Modus immer auf Grundzustand stellen)
			if (_Blinken_Licht_An > 0) { // Blinken? Dann naechsten Einschaltpunkt festlegen
				_Timeout_Licht = Jetzt + _Blinken_Licht_Aus;
			} else						 // sonst timeout auf 0 setzten, damit nichts weiter geschieht.
				_Timeout_Licht = 0; // 
		}
	  } else // Licht ist aus
		if(Jetzt > _Timeout_Licht) { // Jetzt muss Licht zum Blinken angeschaltet werden
			D_PRINTLN("Licht An");
			_Licht_ist_an = false;
			Lcd.noBacklight();
			_Timeout_Licht = Jetzt + _Blinken_Licht_An;		
		} 
  }
}

void Anzeige::Licht_An() {
  _Timeout_Licht = millis() + DAUER_HIGHLIGHT;
  D_PRINT("Licht an, wieder aus bei ");D_PRINTLN(_Timeout_Licht/1000);
  _Licht_ist_an = true;
  Lcd.backlight();
}

void Anzeige::Licht_Aus() {
  D_PRINTLN("Licht aus");
  _Timeout_Licht = 0;
  _Licht_ist_an = false;
  Lcd.noBacklight();
}

void Anzeige::Blinken(unsigned long Dauer_An, unsigned long Dauer_Aus) {
  D_PRINT("Blinkmodus mit "); D_PRINT(Dauer_An);
  D_PRINT("ms (an) und "); D_PRINT(Dauer_Aus);
  D_PRINTLN("ms (aus)");
  _Blinken_Licht_An = Dauer_An;
  _Blinken_Licht_Aus = Dauer_Aus;
  _Timeout_Licht = max(1,_Timeout_Licht); // damit im naechsten tick das Blinken angestossen wird
}

void Anzeige::Blinken_Aus() {
  D_PRINTLN("Blink-Modus aus");
  _Blinken_Licht_An = 0;
  _Timeout_Licht = max(1,_Timeout_Licht); // damit im naechsten tick das Blinken angestossen wird
//  Lcd.noBacklight(); // sicher ist sicher
}

void Anzeige::Werte_Zeile_1(int Liter, int Prozent) {
  _Liter = Liter;
  _Prozent = Prozent;
  _Text_Update = true;
}

void Anzeige::Werte_Zeile_2(unsigned long Verbrauch, int Min, int Akt, int Max) {
  _Min = Min;
  _Akt = Akt;
  _Max = Max;
  _Verbrauch = Verbrauch;
  _Text_Update = true;
}

void Anzeige::Setze_Modus_Zeile_2(Modus_Zeile_2 Modus) {
  _Modus = Modus;
  _Text_Update = true;
  Licht_An();
}
void Anzeige::Modus_Zeile_2_Plus() {
	switch(_Modus) {
	case Verbrauch:
		_Modus = MinMax;
		break;
	case MinMax:
		_Modus = Reset_MinMax;
		break;
	case Reset_MinMax:
		_Modus = Verbrauch;
		break;
	}
	_Text_Update = true;
	Licht_An();
}

void Anzeige::Modus_Zeile_2_Minus() {
	switch(_Modus) {
	case Verbrauch:
		_Modus = Reset_MinMax;
		break;
	case MinMax:
		_Modus = Verbrauch;
		break;
	case Reset_MinMax:
		_Modus = MinMax;
		break;
	}
	_Text_Update = true;
	Licht_An();
}

Anzeige::Modus_Zeile_2 Anzeige::Welcher_Modus_Zeile_2() {
	return _Modus;
}

