#include "MasterStationApp.h"

#include <Logger.h>

MasterStationApp::MasterStationApp(
    uint8_t touchPin,
    IRadioReceiver* receiver,
    IBackend* backend,
    uint8_t dispClk,
    uint8_t dispData,
    uint8_t dispCS,
    uint8_t dispDC,
    uint8_t dispReset,
    unsigned long reportIntervalMs,
    unsigned long backendIntervalMs,
    unsigned long offlineTimeoutMs
) : _lastReport(0),
    _reportIntervalMs(reportIntervalMs),
    _lastPacketReceivedAt(0),
    _offlineTimeoutMs(offlineTimeoutMs),
    _isDisplayOn(true),
    _hasPacket(false),
    _hasSequence(false),
    _lastSequence(0),
    _lastPacket(),
    _display(U8G2_R0, dispClk, dispData, dispCS, dispDC, dispReset),
    _touchButton("DisplayTouch", touchPin, 120, false, false),
    _receiver(receiver),
    _publisher(backend, backendIntervalMs) {}

void MasterStationApp::begin() {
    Logger::begin(115200);
    Logger::info("master-station", "Master station initialization started");

    if (_receiver) {
        _receiver->begin();
    }

    _touchButton.begin();
    _publisher.begin();

    _display.begin();
    _display.setContrast(170);
    _display.sendF("c", 0xA6); // Normal mode: dark background with light pixels on this panel.
    _display.setDrawColor(1);
    _display.setFontMode(1);

    // Welcome screen (fills most of 128x64 area)
    _display.clearBuffer();
    _display.setDrawColor(1);
    _display.setFont(u8g2_font_9x15B_tr);
    _display.drawStr(10, 18, "Weather");
    _display.drawStr(14, 38, "Station");
    _display.setFont(u8g2_font_6x10_tf);
    _display.drawStr(0, 62, "Master v1.0");
    _display.sendBuffer();

    Logger::info("master-station", "Display initialized");
    delay(1500);
}

void MasterStationApp::tick() {
    handleTouchToggle();
    handleRadioReceive();
    handlePeriodicReport();
    handleBackendUpdate();
}

void MasterStationApp::handleTouchToggle() {
    if (!_touchButton.wasPressed()) {
        return;
    }

    _isDisplayOn = !_isDisplayOn;
    _display.setPowerSave(_isDisplayOn ? 0 : 1);
    Logger::info("master-station", _isDisplayOn ? "Display: ON" : "Display: OFF");
}

void MasterStationApp::handleRadioReceive() {
    if (!_receiver) {
        return;
    }

    WeatherPacket packet;
    while (_receiver->receive(packet)) {
        if (_hasSequence) {
            uint8_t expected = static_cast<uint8_t>(_lastSequence + 1);
            if (packet.sequence != expected) {
                String msg = "Sequence gap expected=";
                msg += String(expected);
                msg += " got=";
                msg += String(packet.sequence);
                Logger::warn("master-station", msg.c_str());
            }
        }

        _lastPacket = packet;
        _lastPacketReceivedAt = millis();
        _hasPacket = true;
        _hasSequence = true;
        _lastSequence = packet.sequence;
    }
}

void MasterStationApp::handleBackendUpdate() {
    if (!_hasPacket) {
        return;
    }

    bool isOffline = (millis() - _lastPacketReceivedAt) > _offlineTimeoutMs;
    if (isOffline) {
        return;
    }

    _publisher.publishIfDue(_lastPacket);
}

void MasterStationApp::handlePeriodicReport() {
    if ((millis() - _lastReport) < _reportIntervalMs) {
        return;
    }

    _lastReport = millis();

    if (!_isDisplayOn) {
        return;
    }

    if (!_hasPacket || (millis() - _lastPacketReceivedAt) > _offlineTimeoutMs) {
        _display.clearBuffer();
        _display.setDrawColor(1);
        _display.setFont(u8g2_font_9x15B_tr);
        _display.drawStr(12, 24, "OFFLINE");
        _display.setFont(u8g2_font_6x10_tf);
        _display.drawStr(0, 44, "Waiting for slave");
        _display.drawStr(0, 62, "check radio link");
        _display.sendBuffer();
        return;
    }

    renderPacketOnDisplay(_lastPacket);
}

void MasterStationApp::renderPacketOnDisplay(const WeatherPacket& packet) {
    char buf[28];

    _display.clearBuffer();
    _display.setDrawColor(1);
    _display.setFont(u8g2_font_5x7_tf);

    // Dense 8-row layout to use full 64px height.
    if (packet.climateValid) {
        snprintf(buf, sizeof(buf), "T:%.1fC",
                 (double)packet.temperature, (double)packet.humidity);
        _display.drawStr(0, 7, buf);
        snprintf(buf, sizeof(buf), "H:%.0f%%",
                 (double)packet.humidity);
        _display.drawStr(66, 7, buf);
    } else {
        snprintf(buf, sizeof(buf), "T:--");
        _display.drawStr(0, 7, buf);
        snprintf(buf, sizeof(buf), "H:--");
        _display.drawStr(66, 7, buf);
    }

    if (packet.pressureValid && !isnan(packet.pressure)) {
        snprintf(buf, sizeof(buf), "P: %.1f mmHg", (double)packet.pressure);
    } else {
        snprintf(buf, sizeof(buf), "P: --");
    }
    _display.drawStr(0, 15, buf);

    snprintf(buf, sizeof(buf), "Rain: %s  %s",
             RainSensor::levelToText(static_cast<RainLevel>(packet.rainLevel)),
             packet.isRaining ? "WET" : "DRY");
    _display.drawStr(0, 23, buf);

    snprintf(buf, sizeof(buf), "seq:%u", (unsigned)packet.sequence);
    _display.drawStr(0, 31, buf);

    snprintf(buf, sizeof(buf), "Intensity:%u", (unsigned)packet.rainIntensity);
    _display.drawStr(0, 39, buf);

    bool slaveOnline = (millis() - _lastPacketReceivedAt) <= _offlineTimeoutMs;
    _display.drawStr(0, 55, slaveOnline ? "Online" : "Master online");

    _display.sendBuffer();
}
