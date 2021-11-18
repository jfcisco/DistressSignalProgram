/**
 * Distress Signal Program v1.0
 * 
 * Program that sends distress signal and
 * receives a distress response from rescuers
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
OneButton button1(BTN_1, true);
OneButton button2(BTN_2, true);
OneButton button3(BTN_3, true);
OneButton button4(BTN_4, true);
OneButton button5(BTN_5, true);

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

/** Distress Signal Program  
 *  
    1. Display an option to end distress signal
      1. If it is pressed, return to default state and broadcast zero alert

    2. Get and broadcast the ff. information every 30 seconds until the 
    end option is selected
        1. GPS data
        2. Boat ID
        3. Alert level
    
    3. Continuously listen for rescue response
      1. If response is received display (refresh data every 30 seconds):
        1. Count of rescuers
        2. Nearest Rescue boat data:
        3. ID, GPS and distance
 */

bool inDistress = true;
AlertLevel currentAlertLevel = ALERT_GENERAL; // Hard coded for testing purposes
unsigned long timeLastSignalSent = 0;

void setup() {
  Serial.begin(115200);
  setupOled();
  if (!mesh.init()) {
    Serial.println(F("Failed to initialize mesh network"));
    oled.println(F("Failed to initialize mesh network"));
    oled.display();
    while (true);
  }
  // Add your program's setup code below

  // Set OLED color since that will not change naman
  oled.setTextColor(WHITE);
  
  // Setup button click code
  button1.attachClick(handleButton1Click);
  button2.attachClick(handleButton2Click);
}

void loop() {
  updateButtons();
  updateGps();
  // Add your program's loop code below:
  oled.clearDisplay();
  
  if (inDistress)
  {
    // Show distress state in OLED screen
    oled.setCursor(0, 0);
    oled.setTextSize(1);
    oled.println("Vessel in Distress");

    // When did we last broadcast? 
    // If more than 30 seconds passed, we can broadcast again
    unsigned long currentTime = millis();
    if (currentTime - timeLastSignalSent >= 30000)
    {
      // Retrieve data to be sent (e.g., GPS, Alert Level)
      float gpsLat;
      float gpsLong;
      // Check if GPS data is valid
      if (gps.location.isValid())
      {
        gpsLat = gps.location.lat();
        gpsLong = gps.location.lng();
      }
      else
      {
        // Scenario when gps has no valid gps data
        gpsLat = 999.0;
        gpsLong = 999.0;
      }

      AlertLevel alertlevel = currentAlertLevel;
      // Broadcast a distress signal
      if(mesh.sendDistressSignal(gpsLat, gpsLong, alertlevel))
      {
        // Print debugging
        Serial.printf("Distress signal sent at %lu\n", currentTime);
        timeLastSignalSent = currentTime;
      }
    }

    // While we are also in distress, listen for any distress reponse for us
    if (mesh.listenForDistressResponse())
    {
      DistressResponse response = mesh.getResponse();
      // Assumption: Only data of last responder will appear
      oled.setCursor(0, 30);
      oled.println();
      oled.printf("Responder Number: %d\n", response.address);
      oled.printf("GPS Lat: %f\n", response.address);
      oled.printf("GPS Long: %f\n", response.address);
    }
    
    oled.display();
  }
  else // not in distress
  {
    // Should go back to the default menu
    // As a placeholder, I'll just show the text "Default Menu"
    oled.clearDisplay();
    oled.setCursor(75, 28); // middle of screen tantsa
    oled.setTextSize(2);
    oled.println("Default Menu");
    oled.display();
  }
}

void handleButton1Click()
{
  inDistress = !inDistress;
}

void handleButton2Click()
{
  sendDistressResponse();
}

// FOR TESTING PURPOSES ONLY: Send distress response 
unsigned long lastDistressResponseTime = 0;
unsigned long intervalBetweenDistress = 1000;

void sendDistressResponse()
{
  unsigned long currentTime = millis();
  if (currentTime - lastDistressResponseTime > intervalBetweenDistress) {
    // WARNING: HARDCODED
    // Address should be taken from a distress signal message
    int receiver = 2;
    
    if (mesh.sendDistressResponse(receiver, gps.location.lat(), gps.location.lng())) {
      Serial.println("Sent distress response");
      lastDistressResponseTime = currentTime;
    }
  }
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

void updateButtons() {
  button1.tick();
  button2.tick();
  button3.tick();
  button4.tick();
  button5.tick();
}
