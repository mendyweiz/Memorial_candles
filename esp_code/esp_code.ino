#define BUTTON_PIN 15
#define LED_PIN    16

bool lastButtonState = HIGH;
bool flickering = false;

unsigned long flickerStart = 0;
unsigned long lastToggle = 0;
const unsigned long flickerDuration = 5000; // 5 seconds
const unsigned long flickerInterval = 80;   // flicker speed (ms)

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  Serial.println("Ready");
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // Detect button click (HIGH -> LOW)
  if (lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("Button clicked");
    flickering = true;
    flickerStart = millis();
    lastToggle = millis();
  }

  lastButtonState = buttonState;

  // Handle LED flicker
  if (flickering) {
    unsigned long now = millis();

    if (now - flickerStart >= flickerDuration) {
      flickering = false;
      digitalWrite(LED_PIN, LOW); // turn LED off at end
    } else if (now - lastToggle >= flickerInterval) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastToggle = now;
    }
  }
}
