#include <Wire.h>

int addr = 0x44;


void setup() {
    Wire.begin();
    delay(100);

    setperiodic();

}

void loop() {
}


void setperiodic() {
    int mps = 0x21; /* Measurements per Second. 0x21 = 1 */
    int rep = 0x30; /* Repeatability. High for mps of 1: 0x30 */

    Wire.beginTransmission(addr);
    Wire.write(mps);
    Wire.write(rep);
    Wire.endTransmission();

}

