
#ifndef STUB_TEST
// NewPing: HC-SR04 Sensor Bibliothek
#include <NewPing.h>
#endif

#define MAX_MESS_VERSUCHE   10	// Fehl-Messungen einkalkulieren. Max. Anzahl der Messversuche
#define MIN_GUT_MESSUNG		   5	// Nach x guten Messungen reicht die Genauigkeit

#define MIN_ABSTAND 2                 // Minimaler Abstand Sensor - Wasser[cm]. Check ob Fehlmessung
#define MAX_ABSTAND 200               // Maximaler Abstand Sensor - Wasser[cm]. Reduziert die Zeit bei einer Fehlmessung

#define MIN_IN_MS	US_ROUNDTRIP_CM*MIN_ABSTAND
#define MAX_IN_MS	US_ROUNDTRIP_CM*MAX_ABSTAND


#ifdef STUB_TEST
int Fake_Abstand = (INITIAL_MIN_WASSERABSTAND + INITIAL_MAX_WASSERABSTAND) / 2;
int Fake_Richtung = 1;
#else
// HC-SR04 Sensor
NewPing _HC_SR04(PIN_TRIGGER, PIN_ECHO, MAX_ABSTAND);
#endif


int Messwerte[MAX_MESS_VERSUCHE];

Sensor::Sensor() {
}

int Sensor::Lese_Wasserabstand() {
#ifndef STUB_TEST
  /*  unsigned int mSekunden;
    unsigned int mSekunden_alle = 0;
    byte Gut = 0;
    for (byte i = 0; i < MAX_MESS_VERSUCHE; i++) {
      mSekunden = _HC_SR04.ping();
  	D_P_SEN("Sensor: ");D_P_SENLN(NewPing::convert_cm(mSekunden));
      if ((mSekunden > MIN_IN_MS) && (mSekunden < MAX_IN_MS)) {
        mSekunden_alle += mSekunden;
        Gut++;
        if (Gut >= MIN_GUT_MESSUNG) {
  		D_P_SEN("Gute Messungen: "); D_P_SENLN(Gut);
          return NewPing::convert_cm(mSekunden_alle/ Gut);
  	  }
  	  delay(100);
      }
    }
    D_P_SEN("FAIL: Gute Messungen: "); D_P_SENLN(Gut);
    return NO_ECHO; // nicht genug gute Messungen (Fehler!)
  */

  byte i;
  byte Gute_Messungen = 0;
  long Wert;
  D_P_SEN("Messe ");
  for (i = 0; i < MAX_MESS_VERSUCHE; i++) {
    Wert = _HC_SR04.ping_cm();
    D_P_SEN(i);  D_P_SEN("="); D_P_SEN(Wert);
    if ( (Wert > MIN_ABSTAND) && (Wert < MAX_ABSTAND) ) {
      Messwerte[Gute_Messungen++] = Wert;
      D_P_SEN("+ ");
    } else {
      D_P_SEN("- ");
    }
    delay(100); // Brauche ich wohl nicht, aber sicher ist sicher, gebe dem Sensor ein bisschen Zeit zum Reset
  }
  D_P_SENLN(Gute_Messungen);

  // Schauen ob die Streuung passt und alle Ausreiser wegschmeisen.
  // Erster Schritt: Sind noch "MIN_GUT_MESSUNG"en uebrig --> Ja: Mittelwert berechnen, Nein: Return NO_ECHO
  // Zweiter Schritt: Den Wert finden, der am meisten vom Mittelwert abweicht
  // Dritter Schritt: Falls Abweichung groesser als 2 cm, weg damit und zurück zu Schritt Eins
  // Falls nicht: der Mittelwert passt
  while (Gute_Messungen >= MIN_GUT_MESSUNG ) {
    // Erster Schritt
    Wert = 0;
    for (i = 0; i < Gute_Messungen; i++ ) {
      Wert += Messwerte[i];
    }
    Wert /= Gute_Messungen;
    D_P_SEN("Mittelwert bei "); D_P_SEN(Gute_Messungen); D_P_SEN(" guten Messungen: "); D_P_SENLN(Wert);
    // Zweiter Schritt
    long Abweichung = 0;
    byte Schlechtester_Wert;
    for (i = 0; i < Gute_Messungen; i++ ) {
      if ((Messwerte[i] - Wert) > Abweichung) {
        D_P_SEN("Messwerte["); D_P_SEN(i); D_P_SEN("] - Wert: "); D_P_SENLN(Messwerte[i] - Wert);
        Abweichung = Messwerte[i] - Wert;
        Schlechtester_Wert = i;
      } else if ((Wert - Messwerte[i] ) > Abweichung) {
        D_P_SEN("Wert - Messwerte["); D_P_SEN(i); D_P_SEN(" ]: "); D_P_SENLN(Wert - Messwerte[i]);
        Abweichung = Wert - Messwerte[i];
        Schlechtester_Wert = i;
      }
    }

    // Dritter Schritt
    if (Abweichung > 2) { // Ein Abweichler? Wenn ja, aus dem Array entfernen und Gute_Messungen runterzaehlen
      D_P_SEN("Wert "); D_P_SEN(Schlechtester_Wert); D_P_SEN( "raus"); D_P_SENLN(Messwerte[Schlechtester_Wert]);

      for (i = Schlechtester_Wert; i < Gute_Messungen; i++ ) {
        Messwerte[i] = Messwerte[i + 1];
      }
      Gute_Messungen --;
    } else {
      D_P_SEN("Messung erfolgreich mit "); D_P_SEN(Gute_Messungen); D_P_SEN(" Werten:"); D_P_SENLN(Wert);
      return Wert;
    }

  }
  return NO_ECHO;
#else
  Fake_Abstand += Fake_Richtung;
  if (Fake_Abstand < 90)
    Fake_Richtung = 1;
  if (Fake_Abstand > 110)
    Fake_Richtung = -1;
  return Fake_Abstand;
#endif
}
