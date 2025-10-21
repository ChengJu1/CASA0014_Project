#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 6  // NeoPixel DIN
#define SWITCH_PIN 7    // SS-5GL is pluged in D7
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// state variable
bool ledState = false;       // false = lights off true = lights on
int lastSwitchState = HIGH;  // remember the last state
int red, green, blue;

void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.setBrightness(50);
  pixels.show();

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  Serial.println("Toggle Switch + NeoPixel test started.");
}

void loop() {
  int switchState = digitalRead(SWITCH_PIN);

  // read the last state and do someting when the switch is pressed
  if (switchState == LOW && lastSwitchState == HIGH) {
    ledState = !ledState;  // change the state variable

    if (ledState) {
      // random color
      red = random(0, 5) * 50;
      green = random(0, 5) * 50;
      blue = random(0, 5) * 50;
      pixels.setPixelColor(0, red, green, blue);
      pixels.show();
      Serial.println("LED ON");
    } else {
      pixels.clear();
      pixels.show();
      Serial.println("LED OFF");
    }

    delay(200);  // In case that touch the switch multipal times at once
  }

  lastSwitchState = switchState; // update the lastSwitchState
}