#include <MasterStationApp.h>
#include <WiFiConnection.h>
#include <ThingSpeakBackend.h>
#include <GT24DipReceiver.h>
#include "secrets.h"

// D5: touch button (D6-D10 occupied by GMG12864-06D display)
const int touchPin = 5;
const unsigned long radioBaudRate = 57600;

WiFiConnection wifiConnection(ssid, password);
ThingSpeakBackend tsBackend(wifiConnection, channelID, writeAPIKey);
GT24DipReceiver radioReceiver(Serial1, radioBaudRate);

// Display wiring: CS=10, RSE=9, RS=8, SCL=7, SI=6 (matches MasterStationApp defaults)
MasterStationApp app(
    touchPin,
    &radioReceiver,
    &tsBackend
);

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
