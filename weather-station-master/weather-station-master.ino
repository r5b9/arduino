#include <MasterStationApp.h>
#include <WiFiConnection.h>
#include <ThingSpeakBackend.h>
#include <GT24DipReceiver.h>
#include "secrets.h"

// D3: touch button
// D5: GT-24-DIP CE  (nRF24L01)
// D11: GT-24-DIP CSN
// D12: GT-24-DIP MOSI
// D13: GT-24-DIP SCK
// D4:  GT-24-DIP MISO
// Display: CS=10, RSE=9, RS=8, SCL=7, SI=6
const int touchPin = 3;

WiFiConnection wifiConnection(ssid, password);
ThingSpeakBackend tsBackend(wifiConnection, channelID, writeAPIKey);
GT24DipReceiver radioReceiver; // CE=5, CSN=11, SCK=13, MOSI=12, MISO=4 (defaults)

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
