#include "WeatherStationApp.h"

#include <Logger.h>

WeatherStationApp::WeatherStationApp(
    uint8_t rainDigitalPin,
    uint8_t rainAnalogPin,
    uint8_t touchPin,
    uint8_t sht3xAddress,
    uint8_t bmp580Address,
    float stationElevationMeters,
    IBackend* backend,
    uint8_t lcdAddress,
    uint8_t lcdCols,
    uint8_t lcdRows,
    unsigned long reportIntervalMs,
    unsigned long backendIntervalMs
) : _lastReport(0),
    _reportIntervalMs(reportIntervalMs),
    _isDisplayOn(true),
    _hasPacket(false),
    _lastPacket(),
    _lcd(lcdAddress, lcdCols, lcdRows),
    _touchButton("DisplayTouch", touchPin, 120, false, false),
    _collector(rainDigitalPin, rainAnalogPin, sht3xAddress, bmp580Address, stationElevationMeters),
    _publisher(backend, backendIntervalMs) {}

void WeatherStationApp::begin() {
    Logger::begin(115200);
    Logger::info("weather-station", "Weather Station Initialization Started");

    _collector.begin();
    _touchButton.begin();
    _publisher.begin();

    _lcd.init();
    _lcd.backlight();
    _lcd.setCursor(0, 0);
    _lcd.print("Weather Station");
    _lcd.setCursor(0, 1);
    _lcd.print("Initializing...");

    Logger::info("weather-station", "Sensors initialized");
    delay(1000);
    _lcd.clear();
}

void WeatherStationApp::tick() {
    handleTouchToggle();
    handlePeriodicReport();
    handleBackendUpdate();
}

void WeatherStationApp::handleBackendUpdate() {
    if (!_hasPacket) {
        _lastPacket = _collector.readPacket();
        _hasPacket = true;
    }

    _publisher.publishIfDue(_lastPacket);
}

void WeatherStationApp::handleTouchToggle() {
    if (!_touchButton.wasPressed()) {
        return;
    }

    _isDisplayOn = !_isDisplayOn;
    if (_isDisplayOn) {
        _lcd.backlight();
        _lcd.display();
        Logger::info("weather-station", "Display: ON");
    } else {
        _lcd.noBacklight();
        _lcd.noDisplay();
        Logger::info("weather-station", "Display: OFF");
    }
}

void WeatherStationApp::handlePeriodicReport() {
    if (millis() - _lastReport < _reportIntervalMs) {
        return;
    }

    _lastReport = millis();
    _lastPacket = _collector.readPacket();
    _hasPacket = true;

    logRainReport(_lastPacket);
    if (_isDisplayOn) {
        renderRainOnLcd(_lastPacket);
        _lcd.setCursor(0, 2);
        if (_lastPacket.climateValid) {
            _lcd.print("T: "); _lcd.print(_lastPacket.temperature, 1); _lcd.print("C ");
            _lcd.print("H: "); _lcd.print(_lastPacket.humidity, 0); _lcd.print("%");
        } else {
            _lcd.print("T/H unavailable     ");
        }
    }

    RainLevel level = static_cast<RainLevel>(_lastPacket.rainLevel);
    Logger::log(level == RAIN_HEAVY ? WARN : INFO, "weather-station", RainSensor::levelToStatusText(level));
}

void WeatherStationApp::logRainReport(const WeatherPacket& packet) {
    String logLine = "Digital: ";
    logLine += (packet.isRaining ? "ACTIVE" : "IDLE");
    logLine += " | Raw Intensity: ";
    logLine += packet.rainIntensity;
    Logger::info("weather-station", logLine.c_str());
}

void WeatherStationApp::renderRainOnLcd(const WeatherPacket& packet) {
    _lcd.setCursor(0, 0);
    _lcd.print("Rain: ");
    _lcd.print(packet.rainIntensity);
    if (packet.pressureValid && !isnan(packet.pressure)) {
        _lcd.print(" P:");
        _lcd.print(packet.pressure, 3);
    }
    _lcd.print("      "); // Clear remaining

    _lcd.setCursor(0, 1);
    if (packet.isRaining) {
        _lcd.print("Status: Raining  ");
    } else {
        _lcd.print("Status: Clear    ");
    }

    _lcd.setCursor(0, 3);
    _lcd.print(RainSensor::levelToText(static_cast<RainLevel>(packet.rainLevel)));
    _lcd.print("          ");
}