// Duncan Wilson Oct 2025 - v1 - MQTT messager to vespera

// works with MKR1010

#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <utility/wifi_drv.h>  // library to drive to RGB LED on the MKR1010


#define SECRET_MQTTUSER "Your MQTT Username"
#define SECRET_MQTTPASS "Your MQTT Password"
#define COLOR_SWITCH 7  // SS-5GL Color changing switch
#define POWER_SWITCH 8  // power control switch
#define PRESSURE_1 A1   // two pressure sensors
#define PRESSURE_2 A2

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;
const int NUM_MODES = 6;
const unsigned long ANIMATION_INTERVAL = 100;  // The animation update interval


int colorIndex = 0;
int lastColorSwitch = HIGH;
int lastPowerSwitch = HIGH;
bool powerState = true;  // true = turn the lights on , false = turn the lights off
unsigned long lastAnimationUpdate = 0;
int sunriseStep = 0;
/*********************************************************
 * Lighting Scene Presets (6 home scenarios)
 * 0 = Reading Mode       – bright cool white
 * 1 = Relax Mode         – warm amber
 * 2 = Movie Mode         – dim blue
 * 3 = Sunrise Mode       – gradual warm-to-white fade
 * 4 = Party Mode         – fast random colours
 * 5 = Night Light Mode   – very dim warm orange
 *********************************************************/
int modeIndex = 0;


// create wifi object and mqtt object
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Make sure to update your lightid value below with the one you have been allocated
String lightId = "23";  // the topic id number or user number being used.

// Here we define the MQTT topic we will be publishing data to
String mqtt_topic = "student/CASA0014/luminaire/" + lightId;
String clientId = "23";  // will set once i have mac address so that it is unique

// NeoPixel Configuration - we need to know this to know how to send messages
// to vespera
const int num_leds = 72;
const int payload_size = num_leds * 3;  // x3 for RGB

// Create the byte array to send in MQTT payload this stores all the colours
// in memory so that they can be accessed in for example the rainbow function
byte RGBpayload[payload_size];

void setup() {
  Serial.begin(115200);
  //while (!Serial); // Wait for serial port to connect (useful for debugging)
  Serial.println("Vespera");


  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);

  Serial.print("This device is Vespera ");
  Serial.println(lightId);

  // Connect to WiFi
  startWifi();

  // Connect to MQTT broker
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(2000);
  mqttClient.setCallback(callback);

  Serial.println("Set-up complete");

  pinMode(COLOR_SWITCH, INPUT_PULLUP);
  pinMode(POWER_SWITCH, INPUT_PULLUP);
  pinMode(PRESSURE_1, INPUT);
  pinMode(PRESSURE_2, INPUT);

  Serial.println("NeoPixel + 2 Switches test started.");
}

void loop() {
  // Reconnect if necessary
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  if (WiFi.status() != WL_CONNECTED) {
    startWifi();
  }
  // keep mqtt alive
  mqttClient.loop();

  int colorSwitchState = digitalRead(COLOR_SWITCH);
  int powerSwitchState = digitalRead(POWER_SWITCH);

  // Power control - Handle Power Switch toggle
  if (powerSwitchState == LOW && lastPowerSwitch == HIGH) {
    powerState = !powerState;  // toggle ON/OFF
    Serial.print("Power toggled: ");
    Serial.println(powerState ? "ON" : "OFF");

    if (!powerState) {
      // Turn OFF all LEDs and publish immediately
      send_all_off();
      mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
      Serial.println("All lights OFF – published to MQTT.");
    } else {
      // Turn ON and restore the current lighting mode (with MQTT send inside)
      applyMode();  // this version includes internal MQTT publish
      Serial.print("Power ON – restored mode ");
      Serial.println(modeIndex);
    }
    delay(200);  // debounce
  }

  // Switch 2: cycle lighting scene mode
  if (colorSwitchState == LOW && lastColorSwitch == HIGH && powerState) {
    modeIndex = (modeIndex + 1) % NUM_MODES;
    Serial.print("Lighting mode changed to index: ");
    Serial.println(modeIndex);
    applyMode();
    delay(200);
  }

  // Animation update - runs continuously in loop
  unsigned long now = millis();
  if (powerState && (now - lastAnimationUpdate >= ANIMATION_INTERVAL)) {
    lastAnimationUpdate = now;
    updateAnimatedModes();
  }

  pressure_control();
  lastPowerSwitch = powerSwitchState;
  lastColorSwitch = colorSwitchState;
  delay(50);
}

// Function to update the R, G, B values of a single LED pixel
// RGB can a value between 0-254, pixel is 0-71 for a 72 neopixel strip
void send_RGB_to_pixel(int r, int g, int b, int pixel) {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    // Update the byte array with the specified RGB color pattern
    RGBpayload[pixel * 3 + 0] = (byte)r;  // Red
    RGBpayload[pixel * 3 + 1] = (byte)g;  // Green
    RGBpayload[pixel * 3 + 2] = (byte)b;  // Blue

    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);

    Serial.println("Published whole byte array after updating a single pixel.");
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_RGB_to_pixel*.");
  }
}

void applyBrightnessScale(float factor) {
  if (!powerState) return;
  if (factor < 0.0) factor = 0.0;
  if (factor > 1.0) factor = 1.0;

  byte baseR, baseG, baseB, maxBr;
  getBaseColorForMode(modeIndex, baseR, baseG, baseB, maxBr);

  float rF = baseR * factor;
  float gF = baseG * factor;
  float bF = baseB * factor;

  byte r = (rF > 255) ? 255 : (byte)rF;
  byte g = (gF > 255) ? 255 : (byte)gF;
  byte b = (bF > 255) ? 255 : (byte)bF;

  fill_all(r, g, b);
}

void applyMode() {
  if (!powerState) {
    send_all_off();
    return;
  }

  // Sunrise Mode - initialize
  if (modeIndex == 3) {
    sunriseStep = 0;  // reset step
    byte r = 0;
    byte g = 0;
    byte b = 0;
    fill_all(r, g, b);
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
    Serial.println("Sunrise mode activated ");
    return;
  }

  // Party Mode - initialize
  if (modeIndex == 4) {
    Serial.println("Party mode activated");
    // First random colors
    for (int i = 0; i < num_leds; i++) {
      RGBpayload[i * 3 + 0] = (byte)random(50, 256);
      RGBpayload[i * 3 + 1] = (byte)random(50, 256);
      RGBpayload[i * 3 + 2] = (byte)random(50, 256);
    }
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
    return;
  }
  // Static modes: Reading, Relax, Movie, Night Light
  byte r, g, b, maxBr;
  getBaseColorForMode(modeIndex, r, g, b, maxBr);
  fill_all(r, g, b);
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}

// Return the base RGB + recommended brightness for each static mode
void getBaseColorForMode(int m, byte& r, byte& g, byte& b, byte& maxBrightness) {
  switch (m) {
    case 0:  // Reading
      r = 255;
      g = 255;
      b = 200;
      maxBrightness = 220;
      break;
    case 1:  // Relax
      r = 255;
      g = 150;
      b = 80;
      maxBrightness = 150;
      break;
    case 2:  // Movie
      r = 60;
      g = 80;
      b = 255;
      maxBrightness = 60;
      break;
    case 5:  // Night Light
      r = 255;
      g = 120;
      b = 50;
      maxBrightness = 30;
      break;
    default:  // fallback neutral
      r = 255;
      g = 255;
      b = 255;
      maxBrightness = 180;
      break;
  }
}

void pressure_control() {
  // Cooldown time in milliseconds
  const unsigned long COOLDOWN_MS = 1200;

  // Static variable keeps last trigger time
  static unsigned long lastTriggerTime = 0;

  // Read the two pressure sensor values
  int p1 = analogRead(PRESSURE_1);
  int p2 = analogRead(PRESSURE_2);

  // Print current sensor readings
  Serial.print("Sensor1: ");
  Serial.print(p1);
  Serial.print("   Sensor2: ");
  Serial.println(p2);

  // Check if both sensors are pressed
  bool bothPressed = (p1 < 900) && (p2 < 900);

  // Skip if not pressed or light is off
  if (!(bothPressed && powerState)) {
    Serial.println("⚠️  Waiting for both sensors to be pressed...");
    return;
  }

  // Check cooldown to avoid repeated triggering
  unsigned long now = millis();
  if (now - lastTriggerTime < COOLDOWN_MS) {
    Serial.println("⏳ Cooldown active, skipping update...");
    return;
  }

  // Record current trigger time
  lastTriggerTime = now;

  // Calculate brightness factor from pressure strength
  int strength1 = 1023 - p1;
  int strength2 = 1023 - p2;
  int strengthAvg = (strength1 + strength2) / 2;

  float brightnessFactor = strengthAvg / 1023.0;
  if (brightnessFactor > 1.0) brightnessFactor = 1.0;
  if (brightnessFactor < 0.0) brightnessFactor = 0.0;

  // Static = 0,1,2,5. Animated = 3,4.
  if (modeIndex == 0 || modeIndex == 1 || modeIndex == 2 || modeIndex == 5) {
    applyBrightnessScale(brightnessFactor);
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  } else {
    Serial.println("Brightness squeeze ignored for animated mode.");
  }

  // Print debug information
  Serial.print("Strength1: ");
  Serial.print(strength1);
  Serial.print("  Strength2: ");
  Serial.print(strength2);
  Serial.print("  Avg: ");
  Serial.print(strengthAvg);
  Serial.print("  Brightness factor: ");
  Serial.println(brightnessFactor, 3);

  Serial.print("Scaled RGB = ");

  // Send updated RGB values via MQTT
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);

  Serial.println("Pressure control: brightness updated\n");
}

void send_all_off() {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    // Fill the byte array with the specified RGB color pattern
    for (int pixel = 0; pixel < num_leds; pixel++) {
      RGBpayload[pixel * 3 + 0] = (byte)0;  // Red
      RGBpayload[pixel * 3 + 1] = (byte)0;  // Green
      RGBpayload[pixel * 3 + 2] = (byte)0;  // Blue
    }
    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);

    Serial.println("Published an all zero (off) byte array.");
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_all_off*.");
  }
}

void send_all_random() {
  // Check if the mqttClient is connected before publishing
  if (mqttClient.connected()) {
    // Fill the byte array with the specified RGB color pattern
    for (int pixel = 0; pixel < num_leds; pixel++) {
      RGBpayload[pixel * 3 + 0] = (byte)random(50, 256);  // Red - 256 is exclusive, so it goes up to 255
      RGBpayload[pixel * 3 + 1] = (byte)random(50, 256);  // Green
      RGBpayload[pixel * 3 + 2] = (byte)random(50, 256);  // Blue
    }
    // Publish the byte array
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);

    Serial.println("Published an all random byte array.");
  } else {
    Serial.println("MQTT mqttClient not connected, cannot publish from *send_all_random*.");
  }
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

// fill vespera with full color
void fill_all(byte r, byte g, byte b) {
  for (int i = 0; i < num_leds; i++) {
    RGBpayload[i * 3 + 0] = r;
    RGBpayload[i * 3 + 1] = g;
    RGBpayload[i * 3 + 2] = b;
  }
}

// Update animated modes function
void updateAnimatedModes() {
  if (modeIndex == 3) {
    // Sunrise Mode - continuous looping gradient
    sunriseStep = (sunriseStep + 5) % 256;
    byte r = sunriseStep;
    byte g = (byte)(sunriseStep * 0.6);
    byte b = (byte)(sunriseStep * 0.3);
    fill_all(r, g, b);
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  } else if (modeIndex == 4) {
    // Party Mode - continuous random flashing
    for (int i = 0; i < num_leds; i++) {
      RGBpayload[i * 3 + 0] = (byte)random(50, 256);
      RGBpayload[i * 3 + 1] = (byte)random(50, 256);
      RGBpayload[i * 3 + 2] = (byte)random(50, 256);
    }
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  }
}