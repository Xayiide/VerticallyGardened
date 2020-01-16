
#include <ESP8266WiFi.h>
#include <Wire.h>


#include <SHTSensor.h>
#include <Adafruit_VEML7700.h>
#include <PubSubClient.h>



#define YL69pin   "A0"
#define relaypin  12 /* D6 */

const char *ssid = "POWER_UP";
const char *pass = "RosquillaGl4s3ada!";

SHTSensor sht;
Adafruit_VEML7700 veml = Adafruit_VEML7700();

void connectwifi(const char *, const char *);
void relaystate(int);

float yl69read();
float sht85readhum();
float sht85readtemp();

int veml7700readLux();



void setup() {
	Serial.begin(115200);
	Wire.begin();

	shtinit();
	veml7700init();

	pinMode(YL69pin, INPUT);   // YL-69 pin -> A0
	pinMode(relaypin, OUTPUT); // Relay pin -> D6
	connectwifi(ssid, pass);
}



void loop() {
	int hum = yl69read();
	
}


/* Auxiliary functions */

void connectwifi(const char *ssid, const char *pass) {
	WiFi.begin(ssid, pass);
	Serial.print("[*] Connecting to: ");
	Serial.println(WiFi.SSID());

	int i = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println();
	Serial.println("[+] Connection established");
	Serial.println(WiFi.localIP());
}

void shtinit() {
	sht.init();
	Serial.println("[*] SHT85 inicializado");:wchar_t
}

void veml7700init() {
	veml.begin();
	veml.setGain(VEML7700_GAIN_1_8);
	veml.setIntegrationTime(VEML7700_IT_800MS);

	veml.setLowThreshold (10000);
	veml.setHighThreshold(20000);
	veml.interruptEnable(false);
	Serial.println("[*] VEML7700 inicializado");
}



void relaystate(int state) {
	if (state != 0 || state != 1)
		return;
	
	Serial.print("[+] Estado del relé cambiado a ");
	state == 1 ? println("on") : println("off");

	digitalWrite(D6, state);
}

float yl69read() {
	return analogRead(A0);
}

float sht85readhum() {
	sht.readSample();
	return sht.getHumidity();
}

float sht85readtemp() {
	sht.readSample();
	return sht.getTemperature();
}

int veml7700readLux() {
	return veml.readLux());
}


/*
0  -> D3
1  -> D10
2  -> D4
3  -> D9
4  -> D2
5  -> D1
12 -> D6
13 -> D7
14 -> D5
15 -> D8
16 -> D0
*/