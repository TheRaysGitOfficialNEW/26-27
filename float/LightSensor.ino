#include <Wire.h>
#include "SparkFunISL29125.h"

class LightSensor {
public:

  void init() {
    RGB_sensor.init();
  }

  double getLux() {
    unsigned int red = RGB_sensor.readRed();
    unsigned int green = RGB_sensor.readGreen();
    unsigned int blue = RGB_sensor.readBlue();

    double rLux = (red / MAX_COUNT) * RANGE;
    double gLux = (green / MAX_COUNT) * RANGE;
    double bLux = (blue / MAX_COUNT) * RANGE;

    return (0.2126 * rLux) + (0.7152 * gLux) + (0.0722 * bLux);
  }

private:
  SFE_ISL29125 RGB_sensor;

  double MAX_COUNT = 65535.0;
  double RANGE = 10000.0;
};
