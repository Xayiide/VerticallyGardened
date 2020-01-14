#include <ESP8266WiFi.h>
#include <SHTSensor.h>


#define YL69pin   "A0"
#define relaypin  12 /* D6 */
#define SHT85pin  13 /* D7 */

const char *ssid = "POWER_UP";
const char *pass = "RosquillaGl4s3ada!";

SHTSensor sht;


void connectwifi(const char *, const char *);
void relaystate(int);

int yl69read();
int sht85read();



void setup() {
	Serial.begin(115200);
	delay(10);

	pinMode(YL69pin, INPUT);   // YL-69 pin -> A0
	pinMode(relaypin, OUTPUT); // Relay pin -> D6
	pinMode(SHT85pin, INPUT);  // SHT85 pin -> D7
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

void relaystate(int state) {
	if (state != 0 || state != 1)
		return;

	digitalWrite(D6, state);
}

int yl69read() {
	return analogRead(A0);
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