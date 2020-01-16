#include <ESP8266WiFi.h>
#include <Wire.h>

#include <SHTSensor.h>
#include <Adafruit_VEML7700.h>
#include <PubSubClient.h>



#define YL69PIN   "A0"
#define WATERPIN  "D7"
#define RELAYPIN  12 /* D6 */

#define TBTOKEN "o5RuPZnyKIPz83F5IppJ"

const char *ssid = "POWER_UP";
const char *pass = "RosquillaGl4s3ada!";
char ThingsboardHost[] = "demo.thingsboard.io";


WifiClient wc;
PubSubClient client(wc);
int status = WL_IDLE_STATUS;

SHTSensor sht;
Adafruit_VEML7700 veml = Adafruit_VEML7700();



/* Funciones de inicializacion */
void connectwifi(const char *, const char *);
void shtinit();
void veml7700init();
void yl69init();
void waterinit();

/* Read from sensors */
float sht85readhum();
float sht85readtemp();
float yl69read();
int   waterread();
int   veml7700readLux();

/* Change things */
void relaystate(int);
void reconnect();


void setup() {
	Serial.begin(115200);
	Wire.begin();

	shtinit();
	veml7700init();
	yl69init();
	waterinit();

	pinMode(RELAYPIN, OUTPUT); // Relay pin -> D6
	connectwifi(ssid, pass);
	client.setServer(ThingsboardHost, 1883);
}



void loop() {
	if (!client.connected())
		reconnect();

	float humSuelo    = yl69read();
	float humAmbiente = sht85readhum();
	float temAmbiente = sht85readtemp();
	int   lux         = veml7700readLux();
	int   water       = waterread();
}


/**************************************************************************
 * * * * * * * * * * * * * * * INITIALIZATION * * * * * * * * * * * * * * * 
 **************************************************************************/
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


void shtinit() { /* SHT85 functiona por I2C */
	sht.init();
	Serial.println("[*] SHT85 inicializado");:wchar_t
}


void veml7700init() { /* VEML7700 funciona por I2C. */
	veml.begin();
	veml.setGain(VEML7700_GAIN_1_8);
	veml.setIntegrationTime(VEML7700_IT_800MS);

	veml.setLowThreshold (10000);
	veml.setHighThreshold(20000);
	veml.interruptEnable(false);
	Serial.println("[*] VEML7700 inicializado");
}


void yl69init() {
	pinMode(YL69PIN, INPUT);
}


void waterinit() {
	pinMode(WATERPIN, INPU);
}


/**************************************************************************
 * * * * * * * * * * * * * * READ FROM SENSORS * * * * * * * * * * * * * * 
 **************************************************************************/
float sht85readhum() {
	sht.readSample();
	return sht.getHumidity();
}


float sht85readtemp() {
	sht.readSample();
	return sht.getTemperature();
}


float yl69read() {
	return analogRead(A0);
}


int veml7700readLux() {
	return veml.readLux());
}


int waterread() {
	digitalRead(WATERPIN) == LOW ? return 1 : return 0;
}


/**************************************************************************
 * * * * * * * * * * * * * * CHANGE CONFIGURATION * * * * * * * * * * * * * 
 **************************************************************************/
void relaystate(int state) {
	if (state != 0 || state != 1)
		return;
	
	Serial.print("[+] Estado del relé cambiado a ");
	state == 1 ? println("on") : println("off");

	digitalWrite(RELAYPIN, state);
}


void reconnect() {
	while (!client.connected()) {
		status = WiFi.status();
		if (status != WL_CONNECTED) {
			WiFi.begin(ssid, pass);
			while (WiFi.status() != WL_CONNECTED) {
				delay(500);
				Serial.print(".");
			}
			Serial.printl("\n[+] Connected to AP");
		}
		Serial.println("[*] Connecting to Thingsboard node...");
		if (client.connect("ESP8266", TBTOKEN, NULL)) {
			Serial.println("[+] Done");
		}
		else {
			Serial.println("[!] Failed. Retrying.");
			delay(500);
		}
	}
}




 /* 0  -> D3
    1  -> D10
    2  -> D4
    3  -> D9
    4  -> D2
    5  -> D1
    12 -> D6
    13 -> D7
    14 -> D5
    15 -> D8
    16 -> D0 */