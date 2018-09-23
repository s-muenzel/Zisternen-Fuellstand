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

  _Editier_Modus = false;

  _Text_Update = false;
}

void Anzeige::begin() {
  Lcd.begin();
  delay(100);
  Lcd.noBacklight();
  Lcd.noCursor();
  Lcd.noBlink();
  _Text_Update = true;
}

void Anzeige::Tausender(int n) {
  if (n < 1000) {
    Lcd.print(n);
  } else {
    Lcd.print(n / 1000);
    Lcd.print(".");
    int m = n % 1000;
    if (m < 10)
      Lcd.print("00");
    else if (m < 100)
      Lcd.print("0");
    Lcd.print(m);
  }
}

void Anzeige::tick() {
  // Text
  if (	_Text_Update) {
    Lcd.clear();
    Lcd.setCursor(0, 0);
    Lcd.print("Rest:"); Tausender(_Liter); Lcd.print("l");
    Lcd.setCursor(12, 0);
    Lcd.print(_Prozent); Lcd.print("%");
    D_P_ANZ("Rest:"); D_P_ANZ(_Liter); D_P_ANZ("l "); D_P_ANZ(_Prozent); D_P_ANZLN("%");
    switch (_Modus) {
      case Verbrauch:
        D_P_ANZ("Total: "); D_P_ANZ(_Verbrauch); D_P_ANZLN(" l");
        Lcd.setCursor(0, 1);
        Lcd.print("Total: "); Tausender(_Verbrauch); Lcd.print(" l");
        break;
      case Min:
        Lcd.setCursor(0, 1);
        D_P_ANZ("Total: "); D_P_ANZ(_Verbrauch); D_P_ANZLN(" l");
        D_P_ANZ(_Min); D_P_ANZ("  (<"); D_P_ANZ(_Akt); D_P_ANZ("<"); D_P_ANZ(_Max); D_P_ANZLN(")");
        Lcd.print(_Min); Lcd.print("  (<"); Lcd.print(_Akt); Lcd.print("<"); Lcd.print(_Max); Lcd.print(")");
        Lcd.setCursor(0, 1);
        break;
      case Max:
        Lcd.setCursor(0, 1);
        Lcd.print("(");
        Lcd.print(_Min);
        Lcd.print("<");
        Lcd.print(_Akt);
        Lcd.print("<)  ");
        Lcd.print(_Max);
        Lcd.setCursor(10, 1);
        break;
      case MinMax_Auto:
        Lcd.setCursor(0, 1);
        Lcd.print("Min/Max Auto? ");
        switch (_Min_Max_Auto) {
          case keiner:
            Lcd.print("N");
            break;
          case oben:
            Lcd.print("O");
            break;
          case unten:
            Lcd.print("U");
            break;
          case beide:
            Lcd.print("B");
            break;
        }
        Lcd.setCursor(15, 1);
        break;
      case Leer:
        Lcd.setCursor(0, 1);
        Lcd.print("Leer bei: ");
        Tausender(_Leer);
        Lcd.print("l");
        Lcd.setCursor(10, 1);
        break;
      case Fast_Leer:
        Lcd.setCursor(0, 1);
        Lcd.print("Warnung: ");
        Tausender(_Fast_Leer);
        Lcd.print("l");
        Lcd.setCursor(9, 1);
        break;
      case Fehler:
        Lcd.setCursor(0, 1);
        Lcd.print("Sensorfehler !");
        break;
    }
    _Text_Update = false;
  }
  //Licht
  if ( _Timeout_Licht > 0) {
    unsigned long Jetzt = millis();
    if (_Licht_ist_an) {
      if (Jetzt > _Timeout_Licht) { // Licht ist an und muss ausgeschaltet werden
        D_P_ANZLN("Licht Aus");
        _Licht_ist_an = false;
        _Editier_Modus = false;
        Lcd.noBacklight();
        Lcd.noCursor();
        Lcd.noBlink();
        _Modus = Verbrauch; // zurueck in Grundzustand (Im Blink-Modus immer auf Grundzustand stellen)
        if (_Blinken_Licht_An > 0) { // Blinken? Dann naechsten Einschaltpunkt festlegen
          _Timeout_Licht = Jetzt + _Blinken_Licht_Aus;
        } else						 // sonst timeout auf 0 setzten, damit nichts weiter geschieht.
          _Timeout_Licht = 0; //
      }
    } else // Licht ist aus
      if (Jetzt > _Timeout_Licht) { // Jetzt muss Licht zum Blinken angeschaltet werden
        D_P_ANZLN("Licht An");
        _Licht_ist_an = true;
        Lcd.backlight();
        _Timeout_Licht = Jetzt + _Blinken_Licht_An;
      }
  }
}

void Anzeige::Licht_An() {
  _Timeout_Licht = millis() + DAUER_HIGHLIGHT;
  D_P_ANZ("Licht an, wieder aus bei "); D_P_ANZLN(_Timeout_Licht / 1000);
  _Licht_ist_an = true;
  Lcd.backlight();
}

void Anzeige::Licht_Aus() {
  D_P_ANZLN("Licht aus");
  _Timeout_Licht = 0;
  _Licht_ist_an = false;
  _Editier_Modus = false;
  Lcd.noBacklight();
}

void Anzeige::Blinken(unsigned long Dauer_An, unsigned long Dauer_Aus) {
  D_P_ANZ("Blinkmodus mit "); D_P_ANZ(Dauer_An);
  D_P_ANZ("ms (an) und "); D_P_ANZ(Dauer_Aus);
  D_P_ANZLN("ms (aus)");
  if ( _Blinken_Licht_An != Dauer_An ) {
    _Blinken_Licht_An = Dauer_An;
    _Blinken_Licht_Aus = Dauer_Aus;
    _Timeout_Licht = max(1, _Timeout_Licht); // damit im naechsten tick das Blinken angestossen wird
  }
}

void Anzeige::Blinken_Aus() {
  D_P_ANZLN("Blink-Modus aus");
  if (_Blinken_Licht_An > 0) {
    _Blinken_Licht_An = 0;
    _Timeout_Licht = max(1, _Timeout_Licht); // damit im naechsten tick das Blinken angestossen wird
  }
  Lcd.noCursor();
  Lcd.noBlink();
}

void Anzeige::Werte_Wasserstand(unsigned int Liter, byte Prozent) {
  _Liter = Liter;
  _Prozent = Prozent;
  _Text_Update = true;
}

void Anzeige::Werte_Wasserverbrauch(unsigned long Verbrauch, int Akt) {
  _Verbrauch = Verbrauch;
  _Akt = Akt;
  _Text_Update = true;
}

void Anzeige::Werte_WasserAbstand(int Min, int Max) {
  _Min = Min;
  _Max = Max;
  _Text_Update = true;
}

void Anzeige::Werte_WarnLevel(int Leer, int Fast_Leer) {
  _Leer = Leer;
  _Fast_Leer = Fast_Leer;
  _Text_Update = true;
}

void Anzeige::Werte_Min_Max_Auto(AutoModus MM_A) {
  _Min_Max_Auto = MM_A;
  _Text_Update = true;
}


void Anzeige::Setze_Modus_Zeile_2(Modus_Zeile_2 Modus) {
  _Modus = Modus;
  _Text_Update = true;
  Licht_An();
}


void Anzeige::Modus_Zeile_2_Plus() {
  switch (_Modus) {
    case Verbrauch:
      _Modus = Min;
      break;
    case Min:
      _Modus = Max;
      break;
    case Max:
      _Modus = MinMax_Auto;
      break;
    case MinMax_Auto:
      _Modus = Leer;
      break;
    case Leer:
      _Modus = Fast_Leer;
      break;
    case Fast_Leer:
      _Modus = Verbrauch;
      break;
    default:
      _Modus = Verbrauch;
      break;
  }
  _Text_Update = true;
  Licht_An();
}

void Anzeige::Modus_Zeile_2_Minus() {
  switch (_Modus) {
    case Verbrauch:
      _Modus = Fast_Leer;
      break;
    case Min:
      _Modus = Verbrauch;
      break;
    case Max:
      _Modus = Min;
      break;
    case MinMax_Auto:
      _Modus = Max;
      break;
    case Leer:
      _Modus = MinMax_Auto;
      break;
    case Fast_Leer:
      _Modus = Leer;
      break;
    default:
      _Modus = Verbrauch;
      break;
  }
  _Text_Update = true;
  Licht_An();
}

Anzeige::Modus_Zeile_2 Anzeige::Welcher_Modus_Zeile_2() {
  return _Modus;
}

void Anzeige::Editier_Modus(bool An) {
  _Editier_Modus = An;
  if (An) {
    Lcd.cursor();
    Lcd.blink();
  } else {
    Lcd.noCursor();
    Lcd.noBlink();
  }
}
