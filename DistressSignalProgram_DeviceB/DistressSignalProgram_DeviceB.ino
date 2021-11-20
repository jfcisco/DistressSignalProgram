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
#define NODE_ADDRESS 2 // IMPORTANT: NODE_ADDRESS should be unique per device
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

bool inDistress = false;
AlertLevel currentAlertLevel = ALERT_GENERAL; // Hard coded for testing purposes
unsigned long timeLastSignalSent = 0;
DistressResponse lastResponse;
bool receivedResponse = false;

void setup() {
  Serial.begin(115200);
  setupOled();
  setupGps();
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
    // oled.setCursor(0, 0);
    // oled.setTextSize(1);
    // oled.println("Vessel in Distress");

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
        Serial.printf("Valid: GPS (%f, %f)\n", gps.location.lat(), gps.location.lng());
      }
      else
      {
        // Scenario when gps has no valid gps data
        gpsLat = 999.0;
        gpsLong = 999.0;
        Serial.printf("Invalid: GPS (%f, %f)\n", gps.location.lat(), gps.location.lng());
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
      Serial.println("Received response");
      // Assumption: Only data of last responder will appear
      lastResponse = mesh.getResponse();
      receivedResponse = true;
      
      Serial.printf("Responder Number: %u\n", lastResponse.address);
      Serial.printf("GPS Lat: %f\n", lastResponse.address);
      Serial.printf("GPS Long: %f\n", lastResponse.address);
    }

    // oled.invertDisplay(true);

    // Display the beaming circle
    int phase = (millis() / 1000) % 3;
    drawBeamingCircle(phase);
    oled.setCursor(40, 10);
    oled.setTextSize(2);
    oled.print("SOS");
    oled.print(currentAlertLevel, DEC);

    // Continuously print out response if there is one received
    if (receivedResponse)
    {
      oled.setCursor(0, 30);
      oled.setTextSize(1);
      oled.print("Responder Number: ");
      oled.print(lastResponse.address, DEC);
      oled.print("\n");
      oled.print("("); oled.print(lastResponse.gpsLat, 2); oled.print(", "); oled.print(lastResponse.gpsLong, 2); oled.println(")");           
    }
    
    oled.display();
  }
  else // not in distress
  {
    // Should go back to the default menu
    // As a placeholder, I'll just show the text "Default Menu"
    oled.clearDisplay();
    oled.setCursor(0, 0); // middle of screen tantsa
    oled.setTextSize(1);
    oled.println("Default Menu");
    oled.println("Press btn 1 to go to distress signal state");
    oled.println("Press btn 2 to send a distress response");
    
    // For debugging purposes, listen and print out any received distress signals
    if (mesh.listenForDistressSignal())
    {
      DistressSignal dsignal = mesh.getDistessSignal();
      Serial.printf("Received signal from node %u\n", dsignal.address);
      Serial.printf("with GPS: (%f, %f)\n", dsignal.gpsLat, dsignal.gpsLong);
      Serial.printf("Nature of emergency: %d\n", dsignal.alertLevel);
    }

    oled.invertDisplay(false);
    oled.display();
  }
}

void handleButton1Click() {
  inDistress = !inDistress;
}

void handleButton2Click() {
  sendDistressResponse();
}

// FOR TESTING PURPOSES ONLY: Manually send distress response 
void sendDistressResponse() {
  // WARNING: HARDCODED
  // Address should be taken from a distress signal message
  int receiver = 1;
  
  if (mesh.sendDistressResponse(receiver, gps.location.lat(), gps.location.lng())) {
    Serial.println("Sent distress response");
  }
}

// ==============
// GUI Code
// =============

#define CIRCLE_CENTER_X 63
#define CIRCLE_CENTER_Y 31

void drawBeamingCircle(int phase)
{
  switch (phase)
  {
    case 2:
      oled.fillCircle(CIRCLE_CENTER_X, CIRCLE_CENTER_Y, 65, WHITE);
      oled.fillCircle(CIRCLE_CENTER_X, CIRCLE_CENTER_Y, 55, BLACK);
    case 1:
      oled.fillCircle(CIRCLE_CENTER_X, CIRCLE_CENTER_Y, 45, WHITE);
      oled.fillCircle(CIRCLE_CENTER_X, CIRCLE_CENTER_Y, 35, BLACK);
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

void setupGps() {
  gpsSerial.begin(9600);
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
