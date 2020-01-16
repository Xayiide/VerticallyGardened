#include <Wire.h>
#include <SHTSensor.h>


SHTSensor sht;

void SHT85read();

void setup() {
    Wire.begin();
    Serial.begin(115200);
    delay(1000);

    sht.init();

}

void loop() {
    delay(1000);
    SHT85read();
    delay(100);
}

void SHT85read() {
    if (sht.readSample()) {
        Serial.print("[+] Hum: ");
        Serial.println(sht.getHumidity());
        Serial.print("[+] Tem: ");
        Serial.println(sht.getTemperature());
    }
}


