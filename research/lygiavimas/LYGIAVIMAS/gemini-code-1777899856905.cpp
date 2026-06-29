#define STEP_PIN 0
#define DIR_PIN 1

// Greičio nustatymas mikrosekundėmis (800-1200 yra saugu tavo blokeliui)
const int stepDelay = 1000; 

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  // Kadangi naudojame Common Anode schemą, pradinė būsena HIGH
  digitalWrite(STEP_PIN, HIGH); 
}

void loop() {
  // 1. SUKAME Į VIENĄ PUSĘ 10 SEKUNDŽIŲ
  digitalWrite(DIR_PIN, LOW); 
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { // 10000 ms = 10 s
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
  }

  // 2. PAUZĖ 5 SEKUNDĖS
  delay(5000); 

  // 3. SUKAME Į KITĄ PUSĘ 10 SEKUNDŽIŲ
  digitalWrite(DIR_PIN, HIGH);
  startTime = millis();
  while (millis() - startTime < 10000) {
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
  }

  // 4. PAUZĖ 5 SEKUNDĖS PRIEŠ KARTOJANT CIKLĄ
  delay(5000); 
}