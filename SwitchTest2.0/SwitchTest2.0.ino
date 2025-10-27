#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN  6     // NeoPixel DIN
#define COLOR_SWITCH  7     // SS-5GL color chang switch
#define POWER_SWITCH  8     // power control switch
#define NUMPIXELS     1

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int colorIndex = 0;
int lastColorSwitch = HIGH;
int lastPowerSwitch = HIGH;
bool powerState = true;  // true turn the lights on false turn the lights off

uint32_t colors[] = {
  Adafruit_NeoPixel::Color(255, 0, 0),   // red
  Adafruit_NeoPixel::Color(0, 255, 0),   // green
  Adafruit_NeoPixel::Color(0, 0, 255),   // blue
  Adafruit_NeoPixel::Color(255, 255, 0), // yellow
  Adafruit_NeoPixel::Color(255, 0, 255), // pink
  Adafruit_NeoPixel::Color(0, 255, 255)  // cyan
};
const int numColors = sizeof(colors) / sizeof(colors[0]);

void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();
  pixels.show();

  pinMode(COLOR_SWITCH, INPUT_PULLUP);
  pinMode(POWER_SWITCH, INPUT_PULLUP);

  Serial.println("NeoPixel + 2 Switches test started.");
}

void loop() {
  int colorSwitchState = digitalRead(COLOR_SWITCH);
  int powerSwitchState = digitalRead(POWER_SWITCH);
  int pressureValue = analogRead(A0);
  Serial.println(pressureValue);

  // power switch
  if (powerSwitchState == LOW && lastPowerSwitch == HIGH) {
    powerState = !powerState; // change on/off
    Serial.print("Power toggled: ");
    Serial.println(powerState ? "ON" : "OFF");

    if (!powerState) {   // turn the lights off
      pixels.clear();
      pixels.show();
    } else {
      // When the light turns back on, it displays the current color.
      pixels.setPixelColor(0, colors[colorIndex]);
      pixels.show();
    }

    delay(200); // the delay can avoid the system too sensitive
  }

  // switch the color
  if (colorSwitchState == LOW && lastColorSwitch == HIGH && powerState) {
    colorIndex = (colorIndex + 1) % numColors;  // do the color circle
    pixels.setPixelColor(0, colors[colorIndex]);
    pixels.show();

    Serial.print("Color changed to index: ");
    Serial.println(colorIndex);

    delay(200); // same
  }

  lastColorSwitch = colorSwitchState;
  lastPowerSwitch = powerSwitchState;
}