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
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

////////////////////////////////////////////
// Hilfskonstrukt, zum Debuggen
#define DEBUG
#ifdef DEBUG
#define PRINT      if(true)Serial.print
#define PRINTLN    if(true)Serial.println
#else
#define PRINT      if(false)Serial.print
#define PRINTLN    if(false)Serial.println
#endif


////////////////////////////////////////////
// Konstanten
#define EEPROM_MAXW  0                // Addresse im EEPROM wo Maximaler_Wasserstand gespeichert wird
#define EEPROM_MINW  (EEPROM_MAXW + sizeof(Max_Wasserabstand)) // Addresse im EEPROM wo Minimaler_Wasserstand gespeichert wird
#define EEPROM_VERW  (EEPROM_MINW + sizeof(Min_Wasserabstand)) // Addresse im EEPROM wo die genutzte Wassermenge gespeichert wird

#define INITIAL_MAX_WASSERABSTAND 100 // [cm], nur zum initialisieren (wird real etwa bei 10cm sein)
#define INITIAL_MIN_WASSERABSTAND 101 // [cm], nur zum initialisieren (wird real etwa bei 180cm sein)

#define WASSER_VERBRAUCH_SCHWELLE 3   // [cm] Wasserverbrauch nur hochzählen, wenn der Wasserspiegel mindestens um x cm gesunken ist

#define WASSER_VERBRAUCH_UPDATE 1000  // [l] Wasserverbrauch nur neu im EEPROM abspeichern, wenn mindestens 1 Kubikmeter Wasser verbraucht wurde

#define MAX_ABSTAND 200               // Maximaler Abstand Sensor - Wasser[cm]. Reduziert die Zeit bei einer Fehlmessung

#ifdef DEBUG
#define ZYKLUS_DAUER 1000             // Etwa alle Sekunde messen
#else
#define ZYKLUS_DAUER 60000            // Etwa alle Minute messen
#endif

////////////////////////////////////////////
// Die die globalen Variablen und Objekte

NewPing Sensor(4, 5, MAX_ABSTAND);    // TRIGGER, ECHO, MAX_ABSTAND

LiquidCrystal_I2C Lcd(0x27,16,2);     // I2C Adresse 0x27, 16 Zeichen, 2 Zeilen


// Werte
int Aktueller_Wasserabstand;          // [cm] - Zentimeter Abstand zum Sensor 
int Min_Wasserabstand;                // [cm] - Zentimeter Abstand zum Sensor (groeßer als Maximaler_Wasserstand)
int Max_Wasserabstand;                // [cm] - Zentimeter Abstand zum Sensor (kleiner als Minimaler_Wasserstand)
int Letzter_Wasserabstand;            // [cm] - Letzte Messung, bei der der Wasserverbrauch angepasst wurde
unsigned long Wasserverbrauch;        // [l] - aufsummiert die Menge an Wasser, die wir aus der Zisterne entnommen haben
unsigned long Letzter_Wasserverbrauch;// [l] - Wann wurde das EEPROM das letzte Mal geschrieben



///////////////////////////////////////////
// Hier beginnen die Hilfsfunktionen

unsigned int Liter(int Neu, int Alt) {
// Berechnet das vorhandene Wasser in Litern
// Benutzung entweder (Messung neu, Messung alt):                   Wasserverbrauch    (>0 wenn Neu > Alt)
//           oder     (Max_Wasserabstand, Aktueller_Wasserabstand): übriges Restwasser (>0 wenn Aktueller_Wasserabstand < Max_Wasserabstand)
// Durchmesser der Zisterne ist 2m
// Liter = h * pi r^2
//       = (Max_Wasserabstand[cm] - Aktueller_Wasserabstand[cm])/10[dm/cm] * PI * (2/2)^2 *100[m/dm]^2
  return (Neu - Alt) * PI *10;
}

byte Fuellstand() {
// Vorhandenes Wasser in % der Kapazität
  return (Max_Wasserabstand - Aktueller_Wasserabstand)/(Max_Wasserabstand - Min_Wasserabstand);
}

void Wasserverbrauch_Aktualisieren(unsigned int Wasserabstand) {
// Wasserverbrauch erhöhen, wenn seit der letzten Erhöhung der Wasserspiegel um mindestens 3cm gesunken ist (also 30*PI ~= 94 Liter weg sind)
// Dann Letzter_Wasserabstand auf Stand bringen. Letzter_Wasserabstand auch auf Stand bringen, wenn Wasserspiegel genauso viel gestiegen ist
// (Vorgehen soll nur die Mess-Schwankungen ausfiltern)
  if (Aktueller_Wasserabstand-Letzter_Wasserabstand >= WASSER_VERBRAUCH_SCHWELLE) {
    Wasserverbrauch += Liter(Aktueller_Wasserabstand, Letzter_Wasserabstand);
    Letzter_Wasserabstand = Aktueller_Wasserabstand;
  } else if (Aktueller_Wasserabstand-Letzter_Wasserabstand <= -WASSER_VERBRAUCH_SCHWELLE) {
    Letzter_Wasserabstand = Aktueller_Wasserabstand;    
  }
// Wenn mindestens WASSER_VERBRAUCH_UPDATE Liter verbraucht wurden, EEPROM neu schreiben (Filter, um Schreibzyklen zu minimieren)
  if (Wasserverbrauch - Letzter_Wasserverbrauch > WASSER_VERBRAUCH_UPDATE) {
    EEPROM.put(EEPROM_VERW, Wasserverbrauch);
    Letzter_Wasserverbrauch = Wasserverbrauch;
  }
}

void Lese_EEPROM() {
  EEPROM.get(EEPROM_MAXW, Max_Wasserabstand);
  EEPROM.get(EEPROM_MINW, Min_Wasserabstand);
  EEPROM.get(EEPROM_VERW, Wasserverbrauch); 
}

void Initialisiere_EEPROM() {
  Min_Wasserabstand = INITIAL_MIN_WASSERABSTAND; 
  Max_Wasserabstand = INITIAL_MAX_WASSERABSTAND;
  Wasserverbrauch = 0;
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
  if(Max_Wasserabstand == 0) {
    Initialisiere_EEPROM();
  }

  unsigned int mSekunden = Sensor.ping_median(5);
  Letzter_Wasserabstand = Sensor.convert_cm(mSekunden);
  Letzter_Wasserverbrauch = Wasserverbrauch;

// Start des Displays
  Lcd.init();
  Lcd.backlight();
}

void loop() {
  // Messen
  unsigned int mSekunden = Sensor.ping_median(5);
  Aktueller_Wasserabstand = Sensor.convert_cm(mSekunden);
  // Umrechnen
  // ggfs. Min / Max anpassen
  if(Aktueller_Wasserabstand > Max_Wasserabstand) {
    Max_Wasserabstand = Aktueller_Wasserabstand;
    EEPROM.put(EEPROM_MAXW, Max_Wasserabstand);  
  } else if(Aktueller_Wasserabstand < Min_Wasserabstand) {
    Min_Wasserabstand = Aktueller_Wasserabstand;
    EEPROM.put(EEPROM_MINW, Min_Wasserabstand);
  }
  // Wasserverbrauch addieren
  Wasserverbrauch_Aktualisieren(Aktueller_Wasserabstand);
  
  // Darstellen
  Lcd.clear();
  Lcd.setCursor(0,0);
  Lcd.print("Inhalt:");
  Lcd.print(Liter(Max_Wasserabstand, Aktueller_Wasserabstand));
  Lcd.print("l");
  Lcd.print(Fuellstand());
  Lcd.print("%");
  Lcd.setCursor(0,1);
  Lcd.print("Verbrauch:");
  Lcd.print(Wasserverbrauch);
  
  
  // Wenn DEBUG, dann die Werte auf Seriell ausgeben
  PRINT("Abst. Wasser: "); PRINT(Min_Wasserabstand);
  PRINT("cm<"); PRINT(Aktueller_Wasserabstand);
  PRINT("cm("); PRINT(Letzter_Wasserabstand);
  PRINT("cm)<"); PRINT(Max_Wasserabstand);
  PRINTLN("cm");

  PRINT("Verb. Wasser: "); PRINT(Wasserverbrauch);
  PRINT("l (EEPROM: "); PRINT(Letzter_Wasserverbrauch);
  PRINTLN("l)");

  
  delay(ZYKLUS_DAUER);
}


