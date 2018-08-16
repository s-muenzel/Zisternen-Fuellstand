#ifndef ANZEIGE_H
#define ANZEIGE_H

class Anzeige {
  public:
    Anzeige(); // Constructor

    void begin();

    void tick(); // Update, wenn noetig (z.B. Licht an/aus)

    void Licht_An(unsigned long Dauer);
    void Licht_Aus();

    void Blinken(unsigned long Dauer_An, unsigned long Dauer_Aus);
    void Blinken_Aus();

    void Werte_Zeile_1(int Liter, int Prozent);

    void Werte_Zeile_2(unsigned long Verbrauch, int Min, int Akt, int Max);

    typedef enum Modus_Zeile_2_e {
      Verbrauch = 0,
      MinMax,
      Reset_MinMax
    } Modus_Zeile_2;

    void Setze_Modus_Zeile_2(Modus_Zeile_2 Modus);

  private:

    unsigned long _Timeout_Licht;
    unsigned long _Blinken_Licht_An;
    unsigned long _Blinken_Licht_Aus;
    bool _Licht_ist_an;

    int _Liter;
    int _Prozent;
    int _Min;
    int _Akt;
    int _Max;
    unsigned long _Verbrauch;

    Modus_Zeile_2 _Modus;

    bool _Text_Update;
};

#endif // ANZEIGE_H

