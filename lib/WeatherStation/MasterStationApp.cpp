#include "MasterStationApp.h"

#include <Logger.h>

namespace {
int8_t trendDirection(float current, float previous, float epsilon, bool hasBaseline) {
    if (!hasBaseline) {
        return 0;
    }

    float delta = current - previous;
    if (delta > epsilon) {
        return 1;
    }
    if (delta < -epsilon) {
        return -1;
    }
    return 0;
}

void drawTrendTriangle(U8G2& display, int x, int baselineY, int8_t direction) {
    if (direction > 0) {
        // 4-row, 7px-wide symmetric up triangle.
        display.drawHLine(x,     baselineY - 5, 1);
        display.drawHLine(x - 1, baselineY - 4, 3);
        display.drawHLine(x - 2, baselineY - 3, 5);
        display.drawHLine(x - 3, baselineY - 2, 7);
    } else if (direction < 0) {
        // 4-row, 7px-wide symmetric down triangle.
        display.drawHLine(x - 3, baselineY - 5, 7);
        display.drawHLine(x - 2, baselineY - 4, 5);
        display.drawHLine(x - 1, baselineY - 3, 3);
        display.drawHLine(x,     baselineY - 2, 1);
    }
}
}

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
    _hasTempTrend(false),
    _hasHumidityTrend(false),
    _hasPressureTrend(false),
    _lastSequence(0),
    _prevTemperature(0.0f),
    _prevHumidity(0.0f),
    _prevPressure(0.0f),
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
    int8_t tempTrend = 0;
    int8_t humidityTrend = 0;
    int8_t pressureTrend = 0;

    _display.clearBuffer();
    _display.setDrawColor(1);
    _display.setFont(u8g2_font_5x7_tf);

    // Dense 8-row layout to use full 64px height.
    if (packet.climateValid && !isnan(packet.temperature) && !isnan(packet.humidity)) {
        tempTrend = trendDirection(packet.temperature, _prevTemperature, 0.05f, _hasTempTrend);
        humidityTrend = trendDirection(packet.humidity, _prevHumidity, 0.2f, _hasHumidityTrend);

        snprintf(buf, sizeof(buf), "T: %.1fC ", (double)packet.temperature);
        _display.drawStr(0, 7, buf);
        drawTrendTriangle(_display, 0 + _display.getStrWidth(buf) + 2, 7, tempTrend);

        snprintf(buf, sizeof(buf), "H: %.0f%% ", (double)packet.humidity);
        _display.drawStr(66, 7, buf);
        drawTrendTriangle(_display, 66 + _display.getStrWidth(buf) + 2, 7, humidityTrend);
    } else {
        snprintf(buf, sizeof(buf), "T: --");
        _display.drawStr(0, 7, buf);
        snprintf(buf, sizeof(buf), "H: --");
        _display.drawStr(66, 7, buf);
    }

    if (packet.pressureValid && !isnan(packet.pressure)) {
        pressureTrend = trendDirection(packet.pressure, _prevPressure, 0.1f, _hasPressureTrend);
        snprintf(buf, sizeof(buf), "P: %.1f mmHg ", (double)packet.pressure);
    } else {
        snprintf(buf, sizeof(buf), "P: --");
    }
    _display.drawStr(0, 15, buf);
    drawTrendTriangle(_display, _display.getStrWidth(buf) + 2, 15, pressureTrend);

    snprintf(buf, sizeof(buf), "Rain: %s  %s",
             RainSensor::levelToText(static_cast<RainLevel>(packet.rainLevel)),
             packet.isRaining ? "WET" : "DRY");
    _display.drawStr(0, 23, buf);

    snprintf(buf, sizeof(buf), "seq: %u", (unsigned)packet.sequence);
    _display.drawStr(0, 31, buf);

    snprintf(buf, sizeof(buf), "Intensity: %u", (unsigned)packet.rainIntensity);
    _display.drawStr(0, 39, buf);

    bool slaveOnline = (millis() - _lastPacketReceivedAt) <= _offlineTimeoutMs;
    _display.drawStr(0, 55, slaveOnline ? "Online" : "Master online");

    if (packet.climateValid && !isnan(packet.temperature) && !isnan(packet.humidity)) {
        _prevTemperature = packet.temperature;
        _prevHumidity = packet.humidity;
        _hasTempTrend = true;
        _hasHumidityTrend = true;
    }

    if (packet.pressureValid && !isnan(packet.pressure)) {
        _prevPressure = packet.pressure;
        _hasPressureTrend = true;
    }

    _display.sendBuffer();
}
