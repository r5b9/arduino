#ifndef WEATHER_STATION_APP_H
#define WEATHER_STATION_APP_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Sensors.h>
#include <IBackend.h>
#include <CloudPublisher.h>
#include <SensorCollector.h>

class WeatherStationApp {
public:
    WeatherStationApp(
        uint8_t rainDigitalPin,
        uint8_t rainAnalogPin,
        uint8_t touchPin,
        uint8_t sht3xAddress = 0x44,
        uint8_t bmp580Address = BMP5XX_DEFAULT_ADDRESS,
        float stationElevationMeters = 0.0f,
        IBackend* backend = nullptr,
        uint8_t lcdAddress = 0x27,
        uint8_t lcdCols = 20,
        uint8_t lcdRows = 4,
        unsigned long reportIntervalMs = 2000,
        unsigned long backendIntervalMs = 15000
    );

    void begin();
    void tick();

private:
    void handleTouchToggle();
    void handlePeriodicReport();
    void handleBackendUpdate();
    void logRainReport(const WeatherPacket& packet);
    void renderRainOnLcd(const WeatherPacket& packet);

    unsigned long _lastReport;
    unsigned long _reportIntervalMs;
    bool _isDisplayOn;
    bool _hasPacket;
    WeatherPacket _lastPacket;

    LiquidCrystal_I2C _lcd;
    TouchSensor _touchButton;
    SensorCollector _collector;
    CloudPublisher _publisher;
};

#endif