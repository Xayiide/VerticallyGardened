


void setup() {
  pinMode(D4, OUTPUT); /* antenna LED */
  pinMode(D0, OUTPUT); /* cable LED */


  pinMode(D3, INPUT); /* FLASH Button */

}

void loop() {
  int st = digitalRead(D3); /* Read FLASH button */
  digitalWrite(D4, st);
  
}
