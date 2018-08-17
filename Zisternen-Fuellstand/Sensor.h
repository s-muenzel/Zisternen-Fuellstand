#ifndef SENSOR_H
#define SENSOR_H

// Klasse, mit der der HC-SR04-Sensor bedient wird
//#define STUB_TEST


class Sensor {
  public:
    Sensor(); // Constructor

    int Lese_Wasserabstand();

  private:
};

#endif // SENSOR_H

