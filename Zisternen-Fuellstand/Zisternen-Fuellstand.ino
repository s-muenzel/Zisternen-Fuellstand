/* ---------------------------------------------------------------------------
  // Messung des Wasserspiegels, Umrechnnung in Liter und Prozent Füllstand
  // Darstellung in 20x2 Display
  // Wenn Wasserstand unter xxx Warnung durch Blinken
  // ---------------------------------------------------------------------------
*/

#define DEBUG


// EEPROM: Um Füllstand Max / Min zu speichern. Ggfs. um Gesamt-Wasserverbrauch zu merken
#include <EEPROM.h>

// HC-SR04 Sensor (wrapped)
#include "Sensor.h"
// Anzeige (16x2 LCD per I2C)
#include "Anzeige.h"
// Rotary Encoder, basierend auf Arduino gelistete Bibliothek von Matthias Hertel
#include "DrehGeber.h"


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
#define EEPROM_MAXW  8                                         // Addresse im EEPROM wo Maximaler_Wasserstand gespeichert wird
#define EEPROM_MINW  (EEPROM_MAXW + sizeof(Max_Wasserabstand)) // Addresse im EEPROM wo Minimaler_Wasserstand gespeichert wird
#define EEPROM_VERW  (EEPROM_MINW + sizeof(Min_Wasserabstand)) // Addresse im EEPROM wo die genutzte Wassermenge gespeichert wird
#define EEPROM_Z_L   (EEPROM_VERW + sizeof(Wasserverbrauch))   // Addresse im EEPROM wo die genutzte Wassermenge gespeichert wird
#define EEPROM_Z_F_L (EEPROM_Z_L  + sizeof(Zisterne_Leer))     // Addresse im EEPROM wo die genutzte Wassermenge gespeichert wird

#define INITIAL_MAX_WASSERABSTAND 100 // [cm], nur zum initialisieren (wird real etwa bei 10cm sein)
#define INITIAL_MIN_WASSERABSTAND 101 // [cm], nur zum initialisieren (wird real etwa bei 180cm sein)

#define INITIAL_ZISTERNE_LEER			600   // [l] Bei diesem Restwasserstand sollte zumindest die Aussenpumpe nur noch Luft ansaugen
#define INITIAL_ZISTERNE_FAST_LEER	   1200   // [l] Bei diesem Restwasserstand faengt die Anzeige zur Warnung an zu blinken


#define WASSER_VERBRAUCH_SCHWELLE 3   // [cm] Wasserverbrauch nur hochzählen, wenn der Wasserspiegel mindestens um x cm gesunken ist

#define WASSER_VERBRAUCH_UPDATE 1000  // [l] Wasserverbrauch nur neu im EEPROM abspeichern, wenn mindestens x l Wasser verbraucht wurde (1000 == 1 Kubikmeter)


#define PIN_TRIGGER 			 12
#define PIN_ECHO 				 11

#define PIN_BUTTON                4   // Push-Button des Rotary Encoders
#define PIN_ROTARY_A              5   // "A" des Rotary Encoders
#define PIN_ROTARY_B              6   // "B" des Rotary Encoders

#ifdef DEBUG
#define ZYKLUS_DAUER 5000             // Etwa alle 5 Sekunden messen
#else
#define ZYKLUS_DAUER 60000            // Etwa alle Minute messen
#endif

////////////////////////////////////////////
// Die die globalen Variablen und Objekte

// Objekt mit HC-SR04
Sensor _Sensor;

// Objekt, das ein I2C 16x2 LCD ansteuert
Anzeige _Anzeige;

// Der Rotary Encoder
DrehGeber drehRegler(PIN_ROTARY_A, PIN_ROTARY_A, PIN_BUTTON);


// Werte

// Persistente Werte (werden im EEPROM abgelegt)
int Min_Wasserabstand;                // [cm] - Zentimeter Abstand zum Sensor (groeßer als Maximaler_Wasserstand)
int Max_Wasserabstand;                // [cm] - Zentimeter Abstand zum Sensor (kleiner als Minimaler_Wasserstand)
unsigned long Wasserverbrauch;        // [l]  - aufsummiert die Menge an Wasser, die wir aus der Zisterne entnommen haben
unsigned int Zisterne_Leer;           // [l]  - Ab dieser Restwassermenge bewerte ich die Zisterne als "LEER"
unsigned int Zisterner_Fast_Leer;	  // [l]  - Welcher Restwassermenge ist die Warnschwelle


// Transiente Werte
int Aktueller_Wasserabstand;          // [cm] - Zentimeter Abstand zum Sensor
int Letzter_Wasserabstand = 0;        // [cm] - Letzte Messung, bei der der Wasserverbrauch angepasst wurde
unsigned long Naechste_Messung = 0;   // [ms] - Zeitpunkt der nächsten Messung (nach einer Messung + ZYKLUS_DAUER)
unsigned long Letzter_Wasserverbrauch;// [l]  - Wann wurde das EEPROM das letzte Mal geschrieben



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
  return (100 * (Max_Wasserabstand - Aktueller_Wasserabstand)) / (Max_Wasserabstand - Min_Wasserabstand);

}

bool Wasserverbrauch_Aktualisieren() {
  // Wasserverbrauch erhöhen, wenn seit der letzten Erhöhung der Wasserspiegel um mindestens 3cm gesunken ist (also 30*PI ~= 94 Liter weg sind)
  // Dann Letzter_Wasserabstand auf Stand bringen. Letzter_Wasserabstand auch auf Stand bringen, wenn Wasserspiegel genauso viel gestiegen ist
  // (Vorgehen soll nur Mess-Schwankungen ausfiltern)
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
  EEPROM.get(EEPROM_MAXW,  Max_Wasserabstand);
  EEPROM.get(EEPROM_MINW,  Min_Wasserabstand);
  EEPROM.get(EEPROM_VERW,  Wasserverbrauch);
  EEPROM.get(EEPROM_Z_L,   Zisterne_Leer);
  EEPROM.get(EEPROM_Z_F_L, Zisterner_Fast_Leer);

  D_PRINT("Lese EEPROM: Min("); D_PRINT(EEPROM_MINW);
  D_PRINT(")="); D_PRINT(Min_Wasserabstand);
  D_PRINT(" Max("); D_PRINT(EEPROM_MAXW);
  D_PRINT(")="); D_PRINT(Max_Wasserabstand);
  D_PRINT(" Verbrauch("); D_PRINT(EEPROM_VERW);
  D_PRINT(")="); D_PRINT(Wasserverbrauch);
  D_PRINT(" Leer=("); D_PRINT(EEPROM_Z_L);
  D_PRINT(")="); D_PRINT(Zisterne_Leer);
  D_PRINT(" Fast_Leer("); D_PRINT(EEPROM_Z_L);
  D_PRINT(")="); D_PRINTLN(Zisterner_Fast_Leer);
}

void Initialisiere_EEPROM() {
  Min_Wasserabstand = INITIAL_MIN_WASSERABSTAND;
  Max_Wasserabstand = INITIAL_MAX_WASSERABSTAND;
  Wasserverbrauch = 0;
  Zisterne_Leer = INITIAL_ZISTERNE_LEER;
  Zisterner_Fast_Leer = INITIAL_ZISTERNE_FAST_LEER;

  D_PRINT("Initialisiere EEPROM: Min("); D_PRINT(EEPROM_MINW);
  D_PRINT(")="); D_PRINT(INITIAL_MIN_WASSERABSTAND);
  D_PRINT(" Max("); D_PRINT(EEPROM_MAXW);
  D_PRINT(")="); D_PRINT(INITIAL_MAX_WASSERABSTAND);
  D_PRINT(" Verbrauch("); D_PRINT(EEPROM_VERW);
  D_PRINT(")="); D_PRINTLN(Wasserverbrauch);

  EEPROM.put(EEPROM_MAXW, Max_Wasserabstand);
  EEPROM.put(EEPROM_MINW, Min_Wasserabstand);
  EEPROM.put(EEPROM_VERW, Wasserverbrauch);
  EEPROM.put(EEPROM_Z_L,   Zisterne_Leer);
  EEPROM.put(EEPROM_Z_F_L, Zisterner_Fast_Leer);
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

  Letzter_Wasserabstand = _Sensor.Lese_Wasserabstand();
  Letzter_Wasserverbrauch = Wasserverbrauch;

  // Start des Displays
  _Anzeige.begin();
}

void loop() {
  // In loop jedes Mal Knopf und Encoder prüfen

  unsigned long Jetzt = millis();

  // DrehRegler auswerten
  if (drehRegler.tick()) {
    switch (drehRegler.getChange()) {
      case DrehGeber::Rotated_Plus:		// Drehung nach rechts
        D_PRINT("Drehung rechts:"); D_PRINTLN(drehRegler.getPosition());
		// Falls "Bereich_Aendern" auf true und Modus "MinMax", "Reset_MinMax", dann entsprechende Werte hochzaehlen
		// Falls Modus == Verbrauch oder Fehler, dann "Bereich_Aendern" auf false und Modus wechseln
		// Falls Modus "MinMax" 
        _Anzeige.Modus_Zeile_2_Plus();
        break;
      case DrehGeber::Rotated_Minus:	// Drehung nach links
        D_PRINT("Drehung links:"); D_PRINTLN(drehRegler.getPosition());
        _Anzeige.Modus_Zeile_2_Minus();
        break;
      case DrehGeber::Button_Pressed:	// Knopf gedrueckt
        D_PRINTLN("Knopf gedrueckt");
		// Falls "Bereich_Aendern" true und Modus "MinMax", dann neuen Wert speichern, Bereich_Aendern auf false
		// Falls Modus == Verbrauch, dann "Bereich_Aendern" auf false
		// Falls Modus != Verbrauch, dann Modus-Abhängig "Bereich_Aendern" umstellen
        _Anzeige.Licht_An();
        break;
      case DrehGeber::Button_Released:	// Knopf losgelassen
        D_PRINTLN("Knopf losgelassen");
        break;
      case DrehGeber::Nothing: // kann nicht vorkommen
      default: // sollte nicht vorkommen
        break;
    }
  }

  // Falls die naechste Messung kurz vor dem millis-Ueberlauf sein sollte, koennte sein, dass nie wieder eine Messung passiert.
  if (Jetzt < ZYKLUS_DAUER)
    Naechste_Messung = ZYKLUS_DAUER;
  // Nur wenn ZYKLUS_DAUER vergangen ist, einen Mess-Zyklus starten
  if (Jetzt > Naechste_Messung ) {
    Naechste_Messung += ZYKLUS_DAUER;

    // Messen
    Aktueller_Wasserabstand = _Sensor.Lese_Wasserabstand();

    if (Aktueller_Wasserabstand == 0) { // Keine gute Messung --> Fehler
      D_PRINTLN("Lesefehler HC-SR04");
      _Anzeige.Setze_Modus_Zeile_2(Anzeige::Fehler);
    } else if (Aktueller_Wasserabstand != Letzter_Wasserabstand) { // Messung gut, und Wert hat sich veraendert

      // Umrechnen
      // ggfs. Min / Max anpassen
      if (Aktueller_Wasserabstand > Max_Wasserabstand) {
        D_PRINT("Max Wasserabstand im EEPROM auf "); D_PRINT(Aktueller_Wasserabstand); D_PRINTLN(" gesetzt");
        Max_Wasserabstand = Aktueller_Wasserabstand;
        EEPROM.put(EEPROM_MAXW, Max_Wasserabstand);
        _Anzeige.Werte_Zeile_2(Wasserverbrauch, Min_Wasserabstand, Aktueller_Wasserabstand, Max_Wasserabstand);
      } else if (Aktueller_Wasserabstand < Min_Wasserabstand) {
        D_PRINT("Min Wasserabstand im EEPROM auf "); D_PRINT(Aktueller_Wasserabstand); D_PRINTLN(" gesetzt");
        Min_Wasserabstand = Aktueller_Wasserabstand;
        EEPROM.put(EEPROM_MINW, Min_Wasserabstand);
        _Anzeige.Werte_Zeile_2(Wasserverbrauch, Min_Wasserabstand, Aktueller_Wasserabstand, Max_Wasserabstand);
      }

      unsigned int Restwasser = Liter(Max_Wasserabstand, Aktueller_Wasserabstand);
      // Dem Display die neuen Werte geben
      _Anzeige.Werte_Zeile_1(Restwasser, Fuellstand());

      // Wasserverbrauch addieren
      if (Wasserverbrauch_Aktualisieren())
        _Anzeige.Werte_Zeile_2(Wasserverbrauch, Min_Wasserabstand, Aktueller_Wasserabstand, Max_Wasserabstand);

      if (Restwasser < Zisterne_Leer) {
        _Anzeige.Blinken(800, 1200); // schnelles Blinken (0,8s an, 1,2s aus)
      } else if (Restwasser < Zisterner_Fast_Leer) {
        _Anzeige.Blinken(1000, 5000); // blinken mit 1s an, 5s aus
      } else
        _Anzeige.Blinken_Aus(); // genug Wasser in der Zisterne, kein Blinken


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
  }
  _Anzeige.tick();
}


