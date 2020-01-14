void setup() {
  pinMode(D6, OUTPUT); /* Relay pin */
  pinMode(D3, INPUT);  /* Flash button */
}

void loop() {
  int st = digitalRead(D3);
  digitalWrite(D6, st);
}
