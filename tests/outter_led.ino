

void setup() {
  pinMode(D6, OUTPUT); /* LED pin */
  pinMode(D4, OUTPUT); /* Antenna LED */
  pinMode(D3, INPUT);  /* FLASH Button */

}

void loop() {
  int st = digitalRead(D3); /* Read FLASH button */
  digitalWrite(D4, st);     /* Write status to Antenna LED */
  digitalWrite(D6, st);     /* Write status to LED pin */

}
