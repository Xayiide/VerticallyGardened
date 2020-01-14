void setup() {
  pinMode(A0, INPUT);
  Serial.begin(115200);
}

void loop() {
  int hum = analogRead(A0);
  Serial.print("[+] Hum: ");
  Serial.println(hum);
  delay(1000);
}
