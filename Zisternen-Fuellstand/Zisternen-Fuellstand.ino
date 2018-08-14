/* ---------------------------------------------------------------------------
  // Messung des Wasserspiegels, Umrechnnung in Liter und Prozent Füllstand
  // Darstellung in 20x2 Display
  // Wenn Wasserstand unter xxx Warnung durch Blinken
  // ---------------------------------------------------------------------------
*/

// EEPROM: Um Füllstand Max / Min zu speichern. Ggfs. um Gesamt-Wasserverbrauch zu merken
#include <EEPROM.h>
// NewPing: HC-SR04 Sensor Bibliothek
#include <NewPing.h>
// Anzeige (16x2 LCD per I2C)
#include "Anzeige.h"
// Rotary Encoder, basierend auf Arduino gelistete Bibliothek von Matthias Hertel
#include "DrehGeber.h"


#define DEBUG
#define STUB_TEST


////////////////////////////////////////////
// Hilfskonstrukt, zum Debuggen
#ifdef DEBUG
#define D_PRINT      if(true)Serial.print
#define D_PRINTLN    if(true)Serial.println
#else
#define D_PRINT      if(false)Serial.print
#define D_PRINTLN    if(false)Serial.println
#endif


////////////////////////////////////////////
// Konstanten
#define EEPROM_MAXW  2                // Addresse im EEPROM wo Maximaler_Wasserstand gespeichert wird
#define EEPROM_MINW  (EEPROM_MAXW + sizeof(Max_Wasserabstand)) // Addresse im EEPROM wo Minimaler_Wasserstand gespeichert wird
#define EEPROM_VERW  (EEPROM_MINW + sizeof(Min_Wasserabstand)) // Addresse im EEPROM wo die genutzte Wassermenge gespeichert wird

#define INITIAL_MAX_WASSERABSTAND 100 // [cm], nur zum initialisieren (wird real etwa bei 10cm sein)
#define INITIAL_MIN_WASSERABSTAND 101 // [cm], nur zum initialisieren (wird real etwa bei 180cm sein)

#define WASSER_VERBRAUCH_SCHWELLE 3   // [cm] Wasserverbrauch nur hochzählen, wenn der Wasserspiegel mindestens um x cm gesunken ist

#define WASSER_VERBRAUCH_UPDATE 1000  // [l] Wasserverbrauch nur neu im EEPROM abspeichern, wenn mindestens 1 Kubikmeter Wasser verbraucht wurde

#define PIN_TRIGGER 12
#define PIN_ECHO 11

#define MAX_ABSTAND 200               // Maximaler Abstand Sensor - Wasser[cm]. Reduziert die Zeit bei einer Fehlmessung

#define PIN_BUTTON                4   // Push-Button des Rotary Encoders
#define PIN_ROTARY_A              5   // "A" des Rotary Encoders
#define PIN_ROTARY_B              6   // "B" des Rotary Encoders

#define BELEUCHTUNGSDAUER         2000// Wie lange soll das Licht anbleiben [ms]

#ifdef DEBUG
#define ZYKLUS_DAUER 5000             // Etwa alle 5 Sekunden messen
#else
#define ZYKLUS_DAUER 60000            // Etwa alle Minute messen
#endif

#ifdef STUB_TEST
int Fake_Abstand = (INITIAL_MIN_WASSERABSTAND + INITIAL_MAX_WASSERABSTAND) / 2;
int Fake_Richtung = 1;
#endif

////////////////////////////////////////////
// Die die globalen Variablen und Objekte

NewPing Sensor(PIN_TRIGGER, PIN_ECHO, MAX_ABSTAND);    // TRIGGER, ECHO, MAX_ABSTAND

Anzeige Display();

DrehGeber drehRegler(PIN_ROTARY_A, PIN_ROTARY_A, PIN_BUTTON);


// Werte
int Aktueller_Wasserabstand;          // [cm] - Zentimeter Abstand zum Sensor
int Min_Wasserabstand;                // [cm] - Zentimeter Abstand zum Sensor (groeßer als Maximaler_Wasserstand)
int Max_Wasserabstand;                // [cm] - Zentimeter Abstand zum Sensor (kleiner als Minimaler_Wasserstand)
int Letzter_Wasserabstand = 0;        // [cm] - Letzte Messung, bei der der Wasserverbrauch angepasst wurde
unsigned long Wasserverbrauch;        // [l] - aufsummiert die Menge an Wasser, die wir aus der Zisterne entnommen haben
unsigned long Letzter_Wasserverbrauch;// [l] - Wann wurde das EEPROM das letzte Mal geschrieben

// Hilfsvariablen
int pos = 0;                          // Position des Rotary-Encoder
unsigned long Licht_Aus = 0;          // Wenn sll das Licht wieder abgeschaltet werden
bool Licht_ist_an = false;            // Ist das Licht aktuell an?
unsigned long Naechste_Messung = 0;   // Zeitpunkt der nächsten Messung (nach einer Messung + ZYKLUS_ZEIT

///////////////////////////////////////////
// Hier beginnen die Hilfsfunktionen

unsigned int Liter(int Neu, int Alt) {
  // Berechnet das vorhandene Wasser in Litern
  // Benutzung entweder (Messung neu, Messung alt):                   Wasserverbrauch    (>0 wenn Neu > Alt)
  //           oder     (Max_Wasserabstand, Aktueller_Wasserabstand): übriges Restwasser (>0 wenn Aktueller_Wasserabstand < Max_Wasserabstand)
  // Durchmesser der Zisterne ist 2m
  // Liter = h * pi r^2
  //       = (Max_Wasserabstand[cm] - Aktueller_Wasserabstand[cm])/10[dm/cm] * PI * (2/2)^2 *100[m/dm]^2
  return (Neu - Alt) * PI * 10;
}

byte Fuellstand() {
  // Vorhandenes Wasser in % der Kapazität
  return (Max_Wasserabstand - Aktueller_Wasserabstand) / (Max_Wasserabstand - Min_Wasserabstand);
}

bool Wasserverbrauch_Aktualisieren() {
  // Wasserverbrauch erhöhen, wenn seit der letzten Erhöhung der Wasserspiegel um mindestens 3cm gesunken ist (also 30*PI ~= 94 Liter weg sind)
  // Dann Letzter_Wasserabstand auf Stand bringen. Letzter_Wasserabstand auch auf Stand bringen, wenn Wasserspiegel genauso viel gestiegen ist
  // (Vorgehen soll nur die Mess-Schwankungen ausfiltern)
  bool Update = false;
  if (Aktueller_Wasserabstand - Letzter_Wasserabstand >= WASSER_VERBRAUCH_SCHWELLE) {
    Wasserverbrauch += Liter(Aktueller_Wasserabstand, Letzter_Wasserabstand);
    Letzter_Wasserabstand = Aktueller_Wasserabstand;
    Update = true;
  } else if (Aktueller_Wasserabstand - Letzter_Wasserabstand <= -WASSER_VERBRAUCH_SCHWELLE) {
    Letzter_Wasserabstand = Aktueller_Wasserabstand;
    Update = true;
  }
  // Wenn mindestens WASSER_VERBRAUCH_UPDATE Liter verbraucht wurden, EEPROM neu schreiben (Filter, um Schreibzyklen zu minimieren)
  if (Wasserverbrauch - Letzter_Wasserverbrauch > WASSER_VERBRAUCH_UPDATE) {
    EEPROM.put(EEPROM_VERW, Wasserverbrauch);
    Letzter_Wasserverbrauch = Wasserverbrauch;
  }
  return Update;
}

void Lese_EEPROM() {
  EEPROM.get(EEPROM_MAXW, Max_Wasserabstand);
  EEPROM.get(EEPROM_MINW, Min_Wasserabstand);
  EEPROM.get(EEPROM_VERW, Wasserverbrauch);
  D_PRINT("Lese EEPROM: Min("); D_PRINT(EEPROM_MINW);
  D_PRINT(")="); D_PRINT(Min_Wasserabstand);
  D_PRINT(" Max("); D_PRINT(EEPROM_MAXW);
  D_PRINT(")="); D_PRINT(Max_Wasserabstand);
  D_PRINT(" Verbrauch("); D_PRINT(EEPROM_VERW);
  D_PRINT(")="); D_PRINTLN(Wasserverbrauch);
}

void Initialisiere_EEPROM() {
  Min_Wasserabstand = INITIAL_MIN_WASSERABSTAND;
  Max_Wasserabstand = INITIAL_MAX_WASSERABSTAND;
  Wasserverbrauch = 0;

  D_PRINT("Initialisiere EEPROM: Min("); D_PRINT(EEPROM_MINW);
  D_PRINT(")="); D_PRINT(INITIAL_MIN_WASSERABSTAND);
  D_PRINT(" Max("); D_PRINT(EEPROM_MAXW);
  D_PRINT(")="); D_PRINT(INITIAL_MAX_WASSERABSTAND);
  D_PRINT(" Verbrauch("); D_PRINT(EEPROM_VERW);
  D_PRINT(")="); D_PRINTLN(Wasserverbrauch);

  EEPROM.put(EEPROM_MAXW, Max_Wasserabstand);
  EEPROM.put(EEPROM_MINW, Min_Wasserabstand);
  EEPROM.put(EEPROM_VERW, Wasserverbrauch);
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
#endif

  // Initialsieren der Werte, aus EEPROM und Sensor
  Lese_EEPROM();
  // Wirklich Initial: Falls noch nie Max/Min-Werte gesetzt wurden, einmalig setzen
  if (Max_Wasserabstand == 0) {
    Initialisiere_EEPROM();
  }

#ifndef STUB_TEST
  unsigned int mSekunden = Sensor.ping_median(5);
  Letzter_Wasserabstand = Sensor.convert_cm(mSekunden);
#else
  Letzter_Wasserabstand = Fake_Abstand;
#endif
  Letzter_Wasserverbrauch = Wasserverbrauch;

  // Start des Displays
  Display.begin();

  //// Knopf des Rotary Encoders bereit machen
  //  pinMode(PIN_BUTTON,INPUT);

}

void loop() {
  // In loop jedes Mal Knopf und Encoder prüfen
  // Dann, nur wenn ZYKLUS_DAUER vergangen ist, einen Mess-Zyklus starten

  unsigned long Jetzt = millis();

  // DrehRegler auswerten
  if (drehRegler.tick()) {
    switch (drehRegler.getChange()) {
      case DrehGeber::Rotated: 			// Drehung
        D_PRINT("Drehung:"); D_PRINTLN(drehRegler.getPosition());
        Display.Licht_An(BELEUCHTUNGSDAUER);
        break;
      case DrehGeber::Button_Pressed:	// Knopf gedrueckt
        D_PRINTLN("Knopf gedrueckt");
        Display.Licht_An(BELEUCHTUNGSDAUER);
        break;
      case DrehGeber::Button_Released:	// Knopf losgelassen
        D_PRINTLN("Knopf losgelassen");
        break;
      case DrehGeber::Nothing: // kann nicht vorkommen
      default: // sollte nicht vorkommen
        break;
    }
  }

  if (Jetzt > Naechste_Messung ) {
    Naechste_Messung += ZYKLUS_DAUER;

    // Messen
#ifndef STUB_TEST
    unsigned int mSekunden = Sensor.ping_median(5);
    Aktueller_Wasserabstand = Sensor.convert_cm(mSekunden);
#else
    Fake_Abstand += Fake_Richtung;
    if (Fake_Abstand < 80)
      Fake_Richtung = 1;
    if (Fake_Abstand > 120)
      Fake_Richtung = -1;
    Aktueller_Wasserabstand = Fake_Abstand;

#endif
    // Umrechnen
    // ggfs. Min / Max anpassen
    if (Aktueller_Wasserabstand > Max_Wasserabstand) {
      Max_Wasserabstand = Aktueller_Wasserabstand;
      EEPROM.put(EEPROM_MAXW, Max_Wasserabstand);
      Display.Werte_Zeile_2(Wasserverbrauch, Min_Wasserabstand, Aktueller_Wasserabstand, Max_Wasserabstand);
    } else if (Aktueller_Wasserabstand < Min_Wasserabstand) {
      Min_Wasserabstand = Aktueller_Wasserabstand;
      EEPROM.put(EEPROM_MINW, Min_Wasserabstand);
      Display.Werte_Zeile_2(Wasserverbrauch, Min_Wasserabstand, Aktueller_Wasserabstand, Max_Wasserabstand);
    }

    // Darstellen
    Display.Werte_Zeile_1(Liter(Max_Wasserabstand, Aktueller_Wasserabstand), Fuellstand());

    // Wasserverbrauch addieren
    if (Wasserverbrauch_Aktualisieren())
      Display.Werte_Zeile_2(Wasserverbrauch, Min_Wasserabstand, Aktueller_Wasserabstand, Max_Wasserabstand);


    // Wenn DEBUG, dann die Werte auf Seriell ausgeben
    D_PRINT("Abst. Wasser: "); D_PRINT(Min_Wasserabstand);
    D_PRINT("cm<"); D_PRINT(Aktueller_Wasserabstand);
    D_PRINT("cm("); D_PRINT(Letzter_Wasserabstand);
    D_PRINT("cm)<"); D_PRINT(Max_Wasserabstand);
    D_PRINTLN("cm");

    D_PRINT("Verb. Wasser: "); D_PRINT(Wasserverbrauch);
    D_PRINT("l (EEPROM: "); D_PRINT(Letzter_Wasserverbrauch);
    D_PRINTLN("l)");

  }
  Display.tick();
}


