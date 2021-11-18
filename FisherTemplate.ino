/**
 * FisherNet Program Template v2.0
 * 
 * Standard template to make programming 
 * easier for everyone.
 */

// =============
// Pin Setup
// =============
//  IMPORTANT!! Uncomment the device you are using
// #define LILYGO
#define EGIZMO

#include "PinAssignments.h"
// Reminder: Use the name (BTN_1) instead of writing the pin number directly (i.e., 32).
// Doing this ensures that your code will run on both LILYGO and EGIZMO devices

// ==================
// Libraries Setup
// ==================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "TinyGPS++.h"
#include <OneButton.h>
#include <SoftwareSerial.h>
#include "FisherMesh.h"

// Setup a new OneButton pins
OneButton blue(BTN_1, true);
OneButton green(BTN_2, true);
OneButton red(BTN_3, true);
OneButton pink(BTN_4, true);
OneButton yellow(BTN_5, true);

// Setup OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Setup GPS
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// Setup mesh network
#define NODE_ADDRESS 1 // IMPORTANT: NODE_ADDRESS should be unique per device
#define LORA_FREQUENCY 433.0 // Frequency in MHz. Different for SG!
FisherMesh mesh(NODE_ADDRESS, LORA_FREQUENCY);

// =================
// Main Program
// =================

void setup() {
  Serial.begin(115200);
  setupOled();
  // Add your program's setup code below:
  
}

void loop() {
  updateGps();
  // Add your program's loop code below:
  
}

// =================
// Helper Functions
// =================
void setupOled() {
  // Initialize OLED display with address 0x3C for 128x64
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
}

// Updates gps with data from the module
void updateGps() {
  while (gpsSerial.available() > 0)
    gps.encode(gpsSerial.read());
}
