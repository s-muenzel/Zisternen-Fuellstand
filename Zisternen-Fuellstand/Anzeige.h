#ifndef ANZEIGE_H
#define ANZEIGE_H

// DAUER_HIGHLIGHT ist die Zeit, die das Licht angeschaltet wird [ms]
// ebenso die Zeit, nach der der Modus in den Grundzustand (Verbrauch) zurückfällt
#define DAUER_HIGHLIGHT         2000

class Anzeige {
  public:
    Anzeige(); // Constructor

    void begin();

    void tick(); // Update, wenn noetig (z.B. Licht an/aus)

    void Licht_An();
    void Licht_Aus();

    void Blinken(unsigned long Dauer_An, unsigned long Dauer_Aus);
    void Blinken_Aus();

    void Werte_Zeile_1(unsigned int Liter, byte Prozent);

    void Werte_Zeile_2(unsigned long Verbrauch, int Min, int Akt, int Max);

    typedef enum Modus_Zeile_2_e {
      Verbrauch = 0,
      Min,
	  Max,
      MinMax_Auto,
	  Leer,
	  Fast_Leer,
      Fehler
    } Modus_Zeile_2;

    void Setze_Modus_Zeile_2(Modus_Zeile_2 Modus);
    void Modus_Zeile_2_Plus();
    void Modus_Zeile_2_Minus();
    Modus_Zeile_2 Welcher_Modus_Zeile_2();

  private:

    unsigned long _Timeout_Licht;
    unsigned long _Blinken_Licht_An;
    unsigned long _Blinken_Licht_Aus;
    bool _Licht_ist_an;

    unsigned int _Liter;
    byte _Prozent;
    int _Min;
    int _Akt;
    int _Max;
    unsigned long _Verbrauch;

    Modus_Zeile_2 _Modus;

    bool _Text_Update;
};

#endif // ANZEIGE_H

