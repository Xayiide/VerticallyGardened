#include <ESP8266WiFi.h>       /* WiFi Client        */
#include <Wire.h>              /* I2C                */
#include <NTPClient.h>         /* NTP Protocol. Time */
#include <WiFiUdp.h>           /* WiFi UDP support   */

#include <ESP8266mDNS.h>       /* DNS resolution     */
#include <ArduinoOTA.h>        /* OTA                */

#include <SHTSensor.h>         /* SHT85              */
#include <Adafruit_VEML7700.h> /* VEML7700           */
#include <PubSubClient.h>      /* MQTT               */





#define YL69PIN   A0
#define WATERPIN  D7
#define RELAYPIN  12 /* D6 */
#define FLASHPIN  D3

#define RELAYH 9 /* Hora a la que se activa el rele */
#define RELAYM 0 /* Minuto                          */
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

unsigned long actTime   = 0; /* Guarda el tiempo en el que se activa el rele */
unsigned int  lastwater = 0; /* Guarda el valor de la última lectura de agua */

const char *ssid        = "POWER_UP";
const char *pass        = "RosquillaGl4s3ada!";
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

/* Print banner */
void printbanner();
void printbanner2();


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

	printbanner2();

	pinMode(RELAYPIN, OUTPUT); // Relay pin -> D6
	pinMode(D3, INPUT);        // Flash Button
	pinMode(D4, OUTPUT);
	connectwifi(ssid, pass);

	// Default values:
	// ArduinoOTA.setPort(8266);
	// ArduinoOTA.setHostname("esp8266-[ChipID]");
	// ArduinoOTA.setPassword("");
	// ArduinoOTA.setPasswordHash(MD5(""));

	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH)
			type = "sketch";
		else
			type = "filesystem";

		Serial.println("Start updating " + type);
	});

	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});

	ArduinoOTA.onError([] (ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
	});

	ArduinoOTA.begin();

	shtinit();
	veml7700init();
	yl69init();
	waterinit();

	client.setServer(ThingsboardHost, 1883); /* Set el published de MQTT */
	tc.begin(); /* Inicia cliente de NTP */
}


void loop() {
	if (!client.connected())
		reconnect();

	ArduinoOTA.handle();

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

  int st = 0;
  st = digitalRead(FLASHPIN);
  digitalWrite(D4, st);
  relaystate(st);
	delay(2000);
	Serial.print("\n\n\n\n");
}


/**************************************************************************
 * * * * * * * * * * * * * * * INITIALIZATION * * * * * * * * * * * * * * * 
 **************************************************************************/
void connectwifi(const char *ssid, const char *pass) {
	// WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pass);
	Serial.print("\t\t[*] Connecting to: ");
	Serial.print(WiFi.SSID());

	int i = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println();
	Serial.print("\t\t[+] Connection established. [");
	Serial.print(WiFi.localIP());
  Serial.println("]");
}

void shtinit() { /* SHT85 functiona por I2C */
	sht.init();
	Serial.println("\t\t[*] SHT85 inicializado");
}

void veml7700init() { /* VEML7700 funciona por I2C. */
	veml.begin();
	veml.setGain(VEML7700_GAIN_1_8);
	veml.setIntegrationTime(VEML7700_IT_800MS);
	veml.setLowThreshold (10000);
	veml.setHighThreshold(20000);
	veml.interruptEnable(false);
	Serial.println("\t\t[*] VEML7700 inicializado");
}

void yl69init() {
	pinMode(YL69PIN, INPUT);
	Serial.println("\t\t[*] YL69 inicializado");
}

void waterinit() {
	pinMode(WATERPIN, INPUT);
	Serial.println("\t\t[*] Water inicializado");
}


/**************************************************************************
 * * * * * * * * * * * * * * READ FROM SENSORS * * * * * * * * * * * * * * 
 **************************************************************************/
float sht85readhum() {
	sht.readSample();
	float hum = sht.getHumidity();
	if (hum <= HUMLOWTH) {
		Serial.println("\t[!!] El ambiente es demasiado seco.");
	}
	else if (hum >= HUMHIGHTH) {
		Serial.println("\t[!!] El ambiente es demasiado húmedo.");
	}
	Serial.print("[+] Humedad ambiente: ");
	Serial.println(hum);
	return hum;
}


float sht85readtemp() {
	sht.readSample();
	float temp = sht.getTemperature();
	if (temp <= TEMPLOWTH) {
		Serial.println("\t[!!] La temperatura es demasiado baja.");
	}
	else if (temp >= TEMPHIGHTH) {
		Serial.println("\t[!!] La temperatura es demasiado alta.");
	}
	Serial.print("[+] Temperatura: ");
	Serial.println(temp);
	return temp;
}

/* La humedad va al reves. 1024 significa seco y 0 significa muy mojado.
   Por eso si el valor leido es mayor que el th low es que está seco */
float yl69read() {
	float soilhum = analogRead(YL69PIN);
	if (soilhum >= SOILHUMLOWTH) { /* 800-1024 */
		Serial.println("\t[!!] La humedad del suelo es demasiado baja.");
	}
	else if (soilhum <= SOILHUMHIGHTH) { /* 0-400 */
		Serial.println("\t[!!] La humedad del suelo es demasiado alta.");
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
	lastwater = waterstate;
	return waterstate; 
}


/**************************************************************************
 * * * * * * * * * * * * * * * SEND SENSOR DATA * * * * * * * * * * * * * * 
 **************************************************************************/
void sht85humsend(float humAmbiente) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String humedadAmbiente = "{";
	humedadAmbiente += "\"Humedad_ambiente\":";
	humedadAmbiente += humAmbiente;
	humedadAmbiente += "}";
	humedadAmbiente.toCharArray(attr, ATTRLEN);
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
	client.publish(devapi, attr);
}

void yl69send(float humSuelo) {
	char attr[ATTRLEN];
	memset(attr, 0, sizeof(attr));
	String humedadSuelo = "{";
	humedadSuelo += "\"Humedad_suelo\":";
	humedadSuelo += humSuelo;
	humedadSuelo += "}";
	humedadSuelo.toCharArray(attr, ATTRLEN);
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
	client.publish(devapi, attr);
}


/**************************************************************************
 * * * * * * * * * * * * * * CHANGE CONFIGURATION * * * * * * * * * * * * * 
 **************************************************************************/
void relaystate(int state) {
	if (state != 0 && state != 1) {
		Serial.println("Valor del rele no valido");
		return;
	}
	
	Serial.print("[+] Estado del relé: ");
	state == 1 ? Serial.println("ON") : Serial.println("OFF");

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
		Serial.println("\t\t[*] Connecting to Thingsboard node...");
		if (client.connect("ESP8266", TBTOKEN, NULL)) {
			Serial.println("\t\t[+] Done connecting.");
			Serial.println("\t\t----------------\n");
		}
		else {
			Serial.println("[!] Failed. Retrying.");
			delay(500);
		}
	}
}

void checkrelay() {
	if (tc.getHours()   == RELAYH &&
		tc.getMinutes() == RELAYM) {
			Serial.println("[+] Es la hora de activar el relé");
			relaystate(1);
			actTime = tc.getEpochTime();
	}

	if (RELAYD && actTime != 0) {
		if (tc.getEpochTime() >= actTime + RELAYD) { /* El rele se apaga */
			Serial.println("[+] Es la hora de apagar el relé");
			relaystate(0);
		}
	}
	else {
		if (lastwater) {
			Serial.println("[*] Se cambia el relé por el estado del agua");
			relaystate(0);
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





/* Pa presumir */
void printbanner() {
	Serial.println(" __     __        _   _           _ _       ");
	Serial.println(" \\ \\   \/ \/__ _ __| |_(_) ___ __ _| | |_   _ ");
	Serial.println("  \\ \\ \/ \/ _ \\ \'__| __| |\/ __\/ _` | | | | | |");
	Serial.println("   \\ V \/  __\/ |  | |_| | (_| (_| | | | |_| |");
	Serial.println("    \\_\/ \\___|_|   \\__|_|\\___\\__,_|_|_|\\__, |");
	Serial.println("                                      |___/ ");
	
	Serial.println("   ____               _                     _ ");
	Serial.println("  / ___| __ _ _ __ __| | ___ _ __   ___  __| |");
	Serial.println(" | |  _ \/ _` | \'__\/ _` |\/ _ \\ \'_ \\ \/ _ \\\/ _` |");
	Serial.println(" | |_| | (_| | | | (_| |  __/ | | |  __/ (_| |");
	Serial.println("  \\____|\\__,_|_|  \\__,_|\\___|_| |_|\\___|\\__,_|\n");
	Serial.println("\tPilar Orozco");
	Serial.println("\tSebastián Luchetti");
	Serial.println("\tVerónica Ballero");
	Serial.println("\tEduardo Pozo");
	Serial.println("\n");
}

/* Este es mejor */
void printbanner2() {
	Serial.println(" ██▒   █▓▓█████  ██▀███  ▄▄▄█████▓ ██▓ ▄████▄   ▄▄▄       ██▓     ██▓   ▓██   ██▓");
	Serial.println("▓██░   █▒▓█   ▀ ▓██ ▒ ██▒▓  ██▒ ▓▒▓██▒▒██▀ ▀█  ▒████▄    ▓██▒    ▓██▒    ▒██  ██▒");
	Serial.println(" ▓██  █▒░▒███   ▓██ ░▄█ ▒▒ ▓██░ ▒░▒██▒▒▓█    ▄ ▒██  ▀█▄  ▒██░    ▒██░     ▒██ ██░");
	Serial.println("  ▒██ █░░▒▓█  ▄ ▒██▀▀█▄  ░ ▓██▓ ░ ░██░▒▓▓▄ ▄██▒░██▄▄▄▄██ ▒██░    ▒██░     ░ ▐██▓░");
	Serial.println("   ▒▀█░  ░▒████▒░██▓ ▒██▒  ▒██▒ ░ ░██░▒ ▓███▀ ░ ▓█   ▓██▒░██████▒░██████▒ ░ ██▒▓░");
	Serial.println("   ░ ▐░  ░░ ▒░ ░░ ▒▓ ░▒▓░  ▒ ░░   ░▓  ░ ░▒ ▒  ░ ▒▒   ▓▒█░░ ▒░▓  ░░ ▒░▓  ░  ██▒▒▒ ");
	Serial.println("   ░ ░░   ░ ░  ░  ░▒ ░ ▒░    ░     ▒ ░  ░  ▒     ▒   ▒▒ ░░ ░ ▒  ░░ ░ ▒  ░▓██ ░▒░ ");
	Serial.println("     ░░     ░     ░░   ░   ░       ▒ ░░          ░   ▒     ░ ░     ░ ░   ▒ ▒ ░░  ");
	Serial.println("      ░     ░  ░   ░               ░  ░ ░            ░  ░    ░  ░    ░  ░░ ░     ");
	Serial.println("     ░                                ░                                  ░ ░     ");
	Serial.println("  ▄████  ▄▄▄       ██▀███  ▓█████▄ ▓█████  ███▄    █ ▓█████ ▓█████▄              ");
	Serial.println(" ██▒ ▀█▒▒████▄    ▓██ ▒ ██▒▒██▀ ██▌▓█   ▀  ██ ▀█   █ ▓█   ▀ ▒██▀ ██▌             ");
	Serial.println("▒██░▄▄▄░▒██  ▀█▄  ▓██ ░▄█ ▒░██   █▌▒███   ▓██  ▀█ ██▒▒███   ░██   █▌             ");
	Serial.println("░▓█  ██▓░██▄▄▄▄██ ▒██▀▀█▄  ░▓█▄   ▌▒▓█  ▄ ▓██▒  ▐▌██▒▒▓█  ▄ ░▓█▄   ▌             ");
	Serial.println("░▒▓███▀▒ ▓█   ▓██▒░██▓ ▒██▒░▒████▓ ░▒████▒▒██░   ▓██░░▒████▒░▒████▓              ");
	Serial.println(" ░▒   ▒  ▒▒   ▓▒█░░ ▒▓ ░▒▓░ ▒▒▓  ▒ ░░ ▒░ ░░ ▒░   ▒ ▒ ░░ ▒░ ░ ▒▒▓  ▒              ");
	Serial.println("  ░   ░   ▒   ▒▒ ░  ░▒ ░ ▒░ ░ ▒  ▒  ░ ░  ░░ ░░   ░ ▒░ ░ ░  ░ ░ ▒  ▒              ");
	Serial.println("░ ░   ░   ░   ▒     ░░   ░  ░ ░  ░    ░      ░   ░ ░    ░    ░ ░  ░              ");
	Serial.println("      ░       ░  ░   ░        ░       ░  ░         ░    ░  ░   ░                 ");
	Serial.println("                            ░                                ░                   ");
	Serial.println("\tPilar Orozco");
	Serial.println("\tSebastián Luchetti");
	Serial.println("\tVerónica Ballero");
	Serial.println("\tEduardo Pozo");
	Serial.println("\n");

}


