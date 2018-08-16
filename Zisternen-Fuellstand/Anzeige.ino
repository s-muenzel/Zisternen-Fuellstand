#include <LiquidCrystal_I2C.h>
#include "Anzeige.h"

////////////////////////////////////////////
// Hilfskonstrukt, zum Debuggen
//#ifdef DEBUG
//#define D_PRINT      if(true)Serial.print
//#define D_PRINTLN    if(true)Serial.println
//#else
//#define D_PRINT      if(false)Serial.print
//#define D_PRINTLN    if(false)Serial.println
//#endif

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
  if (_Licht_ist_an && (millis() > _Timeout_Licht)) {
    _Licht_ist_an = false;
    Lcd.noBacklight(); // sicher ist sicher
  }
  //////////////// Licht !!!
}

void Anzeige::Licht_An(unsigned long Dauer) {
  _Timeout_Licht = millis();
  if (_Timeout_Licht + Dauer < _Timeout_Licht) {
    // Ups, genau jetzt ist der Überlauf von millis()
    _Timeout_Licht = Dauer;
  } else {
    _Timeout_Licht += Dauer;
  }
  //	D_PRINT(") an, aus bei ");D_PRINTLN(_Timeout_Licht/1000);
  _Licht_ist_an = true;
  Lcd.backlight(); // sicher ist sicher
}

void Anzeige::Licht_Aus() {
  _Timeout_Licht = 0;
  _Licht_ist_an = false;
  Lcd.noBacklight(); // sicher ist sicher
}

void Anzeige::Blinken(unsigned long Dauer_An, unsigned long Dauer_Aus) {
  _Blinken_Licht_An = Dauer_An;
  _Blinken_Licht_Aus = Dauer_Aus;
  // Blinken anschalten??
}

void Anzeige::Blinken_Aus() {
  _Blinken_Licht_An = 0;
  Lcd.noBacklight(); // sicher ist sicher
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
}

