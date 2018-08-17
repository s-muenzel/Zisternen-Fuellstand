
#ifndef STUB_TEST
// NewPing: HC-SR04 Sensor Bibliothek
#include <NewPing.h>
#endif

#define MAX_MESS_VERSUCHE   5	// Fehl-Messungen einkalkulieren. Max. Anzahl der Messversuche
#define MIN_GUT_MESSUNG		3	// Nach x guten Messungen reicht die Genauigkeit

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


Sensor::Sensor() {
}

int Sensor::Lese_Wasserabstand() {
#ifndef STUB_TEST
  unsigned int mSekunden;
  unsigned int mSekunden_alle = 0;
  byte Gut = 0;
  for (byte i = 0; i < MAX_MESS_VERSUCHE; i++) {
    mSekunden = _HC_SR04.ping();
    if ((mSekunden > MIN_IN_MS) && (mSekunden < MAX_IN_MS)) {
      mSekunden_alle += mSekunden;
      Gut++;
      if (Gut >= MIN_GUT_MESSUNG)
        return NewPing::convert_cm(mSekunden_alle) / MIN_GUT_MESSUNG;
    }
  }
  return NO_ECHO; // nicht genug gute Messungen (Fehler!)
#else
  Fake_Abstand += Fake_Richtung;
  if (Fake_Abstand < 80)
    Fake_Richtung = 1;
  if (Fake_Abstand > 120)
    Fake_Richtung = -1;
  return Fake_Abstand;
#endif
}
