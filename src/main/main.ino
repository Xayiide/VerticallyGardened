#include <ESP8266WiFi.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <SHTSensor.h>
#include <Adafruit_VEML7700.h>
#include <PubSubClient.h>



#define YL69PIN   A0
#define WATERPIN  D7
#define RELAYPIN  12 /* D6 */

#define RELAYH 9
#define RELAYM 0
#define RELAYS 0
#define RELAYD 1800 /* Tiempo que el rele estara activado. 0 si no se usa */

#define ATTRLEN 128 /* Longitud de los atributos que mandamos a thingsboard*/

/* Thresholds de los datos a enviar. Valores limite por encima y por debajo */
#define HUMLOWTH      30
#define HUMHIGHTH     70
#define TEMPLOWTH     10
#define TEMPHIGHTH    40
#define SOILHUMLOWTH  800
#define SOILHUMHIGHTH 400
#define LUXLOWTH      20
#define LUXHIGHTH     6000



#define TBTOKEN "q4M2LoLE5GeWvSvsuFj9"

unsigned long actTime = 0; /* Guarda el tiempo en el que se activa el rele */

const char *ssid        = "SBC";
const char *pass        = "sbc$18-maceta";
const char *devapi      = "v1/devices/me/telemetry";
const long utcoff       = 3600; /* UTC Offset -> Madrid = UTC + 1 */
char  ThingsboardHost[] = "demo.thingsboard.io";

char  ntpServer[]       = "europe.pool.ntp.org";


WiFiUDP   udp;
NTPClient tc(udp, ntpServer, utcoff);

WiFiClient   wc;
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

/* Leer de los sensores */
float sht85readhum();
float sht85readtemp();
float yl69read();
int   waterread();
int   veml7700readLux();

/* Enviar data de sensores a Thingsboard */
void sht85humsend(float);
void sht85tempsend(float);
void yl69send(float);
void watersend(int);
void veml7700luxsend(int);


/* Modificar cosas del sistema */
void relaystate(int);
void reconnect();
void checkrelay();

void setup() {
	Serial.begin(115200);
	Wire.begin();

	shtinit();
	veml7700init();
	yl69init();
	waterinit();

	pinMode(RELAYPIN, OUTPUT); // Relay pin -> D6
	connectwifi(ssid, pass);
	client.setServer(ThingsboardHost, 1883); /* Set el published de MQTT */
	tc.begin(); /* Inicia cliente de NTP */
}



void loop() {
	if (!client.connected())
		reconnect();

	tc.update(); /* Update NTP client time */
	checkrelay(); /* Check if relay should be activated or deactivated */

	float humSuelo    = yl69read();
	float humAmbiente = sht85readhum();
	float temAmbiente = sht85readtemp();
	int   lux         = veml7700readLux();
	int   water       = waterread();

	yl69send(humSuelo);
	sht85humsend(humAmbiente);
	sht85tempsend(temAmbiente);
	veml7700luxsend(lux);
	watersend(water);

	delay(1000);
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
	Serial.println("[*] SHT85 inicializado");
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
	Serial.println("[*] YL69 inicializado");
}


void waterinit() {
	pinMode(WATERPIN, INPUT);
	Serial.println("[*] Water inicializado");
}


/**************************************************************************
 * * * * * * * * * * * * * * READ FROM SENSORS * * * * * * * * * * * * * * 
 **************************************************************************/
/* La humedad va al reves. 1024 significa seco y 0 significa muy mojado.
   Por eso si el valor leido es mayor que el th low es que está seco */
float sht85readhum() {
	sht.readSample();
	float hum = sht.getHumidity();
	if (hum >= HUMLOWTH) {
		Serial.println("[!!] El ambiente es demasiado seco.");
	}
	else if (hum <= HUMHIGHTH) {
		Serial.println("[!!] El ambiente es demasiado húmedo.");
	}
	Serial.print("[+] Humedad ambiente: ");
	Serial.println(hum);
	return hum;
}


float sht85readtemp() {
	sht.readSample();
	float temp = sht.getTemperature();
	if (temp <= TEMPLOWTH) {
		Serial.println("[!!] La temperatura es demasiado baja.");
	}
	else if (temp >= TEMPHIGHTH) {
		Serial.println("[!!] La temperatura es demasiado alta.");
	}
	Serial.print("[+] Temperatura: ");
	Serial.println(temp);
	return temp;
}


float yl69read() {
	float soilhum = analogRead(YL69PIN);
	if (soilhum <= SOILHUMLOWTH) {
		Serial.println("[!!] La humedad del suelo es demasiado baja.");
	}
	else if (soilhum >= SOILHUMHIGHTH) {
		Serial.println("[!!] La humedad del suelo es demasiado alta.");
	}
	Serial.print("[+] Humedad soil: ");
	Serial.println(soilhum);
	return soilhum;
}


int veml7700readLux() {
	int lux = veml.readLux();
	if (lux <= LUXLOWTH) {
		Serial.println("[!!] La cantidad de luz no es suficiente.");
	}
	else if (lux >= LUXHIGHTH) {
		Serial.println("[!!] La cantidad de luz es demasiado alta.");
	}
	Serial.print("[+] Lux: ");
	Serial.println(lux);
	return lux;
}


int waterread() {
	int waterstate = 0;
	if (digitalRead(WATERPIN) == LOW)
		waterstate = 1;
	Serial.print("[+] Agua: ");
	Serial.println(waterstate);
	return waterstate; 
}

/**************************************************************************
 * * * * * * * * * * * * * * * SEND SENSOR DATA * * * * * * * * * * * * * * 
 **************************************************************************/
void sht85humsend(float humAmbiente) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String humedadAmbiente = "{";
	humedadAmbiente += "\"Humedad ambiente\":";
	humedadAmbiente += humAmbiente;
	humedadAmbiente += "}";
	humedadAmbiente.toCharArray(attr, ATTRLEN);
	Serial.println("[+] Sending humedad ambiente");
	client.publish(devapi, attr);
}

void sht85tempsend(float temAmbiente) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String temperatura = "{";
	temperatura += "\"Temperatura\":";
	temperatura += temAmbiente;
	temperatura += "}";
	temperatura.toCharArray(attr, ATTRLEN);
	Serial.println("[+] Sending temperatura ambiente");
	client.publish(devapi, attr);
}

void yl69send(float humSuelo) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String humedadSuelo = "{";
	humedadSuelo += "\"Humedad suelo\":";
	humedadSuelo += humSuelo;
	humedadSuelo += "}";
	humedadSuelo.toCharArray(attr, ATTRLEN);
	Serial.println("[+] Sending humedad soil");
	client.publish(devapi, attr);
}

void watersend(int water) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String agua = "{";
	agua += "\"Agua\":";
	agua += water;
	agua += "}";
	agua.toCharArray(attr, ATTRLEN);
	Serial.println("[+] Sending agua");
	client.publish(devapi ,attr);
}

void veml7700luxsend(int lux) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String luz = "{";
	luz += "\"Luz\":";
	luz += lux;
	luz += "}";
	luz.toCharArray(attr, ATTRLEN);
	Serial.println("[+] Sending lux");
	client.publish(devapi, attr);
}




/**************************************************************************
 * * * * * * * * * * * * * * CHANGE CONFIGURATION * * * * * * * * * * * * * 
 **************************************************************************/
void relaystate(int state) {
	if (state != 0 || state != 1)
		return;
	
	Serial.print("[+] Estado del relé cambiado a ");
	state == 1 ? Serial.println("on") : Serial.println("off");

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
			Serial.println("\n[+] Connected to AP");
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

void checkrelay() {
	if (tc.getHours()   == RELAYH &&
		tc.getMinutes() == RELAYM &&
		tc.getSeconds() == RELAYS) {
			relaystate(1);
			actTime = tc.getEpochTime();
	}

	if (RELAYD) {
		if (tc.getEpochTime() >= actTime + RELAYD) /* El rele se apaga */
			relaystate(0);
	}
	else {
		if (waterread())
			relaystate(0);
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
