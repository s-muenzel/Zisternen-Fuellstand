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
#include <LiquidCrystal_I2C.h>

#define MAX_ABSTAND 200 // Maximum distance (in cm) to ping.

NewPing Sensor(4, 5, MAX_ABSTAND); // TRIGGER, ECHO, MAX_ABSTAND

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
}

void loop() { 
  Serial.print("Entfernung Wasserspiegel: ");
  Serial.print(Sensor.ping_cm());
  Serial.print("cm ");
  Serial.println();
  delay(50); // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.

  
}


