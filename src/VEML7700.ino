#include "Adafruit_VEML7700.h"

Adafruit_VEML7700 veml = Adafruit_VEML7700();

void setup() {
  Serial.begin(115200);

  !veml.begin();

  veml.setGain(VEML7700_GAIN_1_8);
  veml.setIntegrationTime(VEML7700_IT_800MS);


  veml.setLowThreshold(10000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(false);
}

void loop() {
  Serial.print("Lux: "); Serial.println(veml.readLux());
  delay(1500);
}
