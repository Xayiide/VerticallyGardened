#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char *ssid = "POWER_UP";
const char *pass = "RosquillaGl4s3ada!";

void connectwifi(const char *, const char *);




void setup() {
  pinMode(D4, OUTPUT);
  
  Serial.begin(115200);
  delay(10);

  connectwifi(ssid, pass);
}


void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(D4, HIGH);
  delay(1000);
  digitalWrite(D4, LOW);
  delay(1000);
}



void connectwifi(const char *ssid, const char *pass) {
  WiFi.begin(ssid, pass);
  Serial.print("[*] Connecting to: ");
  Serial.println(ssid);
  Serial.println(WiFi.SSID());

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("[+] Connection established: ");
  Serial.println(WiFi.localIP());
}
