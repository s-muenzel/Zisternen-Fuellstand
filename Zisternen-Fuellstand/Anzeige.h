#ifndef ANZEIGE_H
#define ANZEIGE_H


class Anzeige {
  public:
    Anzeige(); // Constructor

    typedef enum AutoModus_e {
      keiner,
      oben,
      unten,
      beide
    } AutoModus;

    typedef enum Modus_Zeile_2_e {
      Verbrauch = 0,
      Min,
      Max,
      MinMax_Auto,
      Leer,
      Fast_Leer,
      Fehler
    } Modus_Zeile_2;


    void begin();

    void tick(); // Update, wenn noetig (z.B. Licht an/aus)

    void Licht_An();
    void Licht_Aus();
    bool ist_Licht_An() {
      return _Licht_ist_an;
    };

    void Blinken(unsigned long Dauer_An, unsigned long Dauer_Aus);
    void Blinken_Aus();

    void Werte_Wasserstand(unsigned int Liter, byte Prozent);
    void Werte_Wasserverbrauch(unsigned long Verbrauch, int Akt);
    void Werte_WasserAbstand(int Min, int Max);
    void Werte_Min_Max_Auto(AutoModus MM_A);
    void Werte_WarnLevel(int Leer, int Fast_Leer);



    void Setze_Modus_Zeile_2(Modus_Zeile_2 Modus);
    void Modus_Zeile_2_Plus();
    void Modus_Zeile_2_Minus();
    Modus_Zeile_2 Welcher_Modus_Zeile_2();

    bool ist_Editier_Modus() {
      return _Editier_Modus;
    };
    void Editier_Modus(bool An);

  private:

    void Tausender(int n); // Ausgabe mit "." bei > 999

    unsigned long _Timeout_Licht;
    unsigned long _Blinken_Licht_An;
    unsigned long _Blinken_Licht_Aus;
    bool _Licht_ist_an;

    unsigned int _Liter;
    byte _Prozent;
    int _Min;
    int _Akt;
    int _Max;
    AutoModus _Min_Max_Auto;
    int _Leer;
    int _Fast_Leer;
    unsigned long _Verbrauch;

    Modus_Zeile_2 _Modus;

    bool _Editier_Modus;

    bool _Text_Update;
};

#endif // ANZEIGE_H

