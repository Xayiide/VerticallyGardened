
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "SBC";
const char* password = "sbc$18-maceta";

#define TOKEN2 "HtsQJrRn80mXL4YHJp6a" //Access token of device Display
#define WATER_SENSOR D6
char ThingsboardHost[] = "demo.thingsboard.io";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
int status = WL_IDLE_STATUS;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("connected to");
  Serial.println(ssid);
  client.setServer( ThingsboardHost, 1883 );
  //pinMode(A0, INPUT); //YL69
  pinMode(WATER_SENSOR, INPUT); //Water_sensor 
}

void loop()
{
if ( !client.connected() ) 
{
    reconnect();
}
  isExposedToWater();
  delay(2000);
}


void isExposedToWater()
{
  boolean waterstate=false;
  if(digitalRead(WATER_SENSOR) == LOW)
  waterstate = true;
  else waterstate= false;
  
  Serial.print("[+] water: ");
  Serial.println(waterstate);
  String payload = "{";
  payload += "\"Agua\":";payload += waterstate;
  payload += "}";

  char attributes[1000];
  payload.toCharArray( attributes, 1000 );
  client.publish( "v1/devices/me/telemetry",attributes);
  Serial.println( attributes );
  
  
 }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
      if ( client.connect("Esp8266", TOKEN2, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.println( " : retrying in 5 seconds]" );
      delay( 500 );
    }
  }
}
