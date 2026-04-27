#ifndef SENSOR_COLLECTOR_H
#define SENSOR_COLLECTOR_H

#include <Arduino.h>
#include <Sensors.h>
#include <WeatherPacket.h>

class SensorCollector {
public:
    SensorCollector(
        uint8_t rainDigitalPin,
        uint8_t rainAnalogPin,
        uint8_t sht3xAddress,
        uint8_t bmp580Address,
        float stationElevationMeters,
        unsigned long environmentRefreshIntervalMs = 5000
    );

    bool begin();
    WeatherPacket readPacket();

private:
    bool readClimate(float& temperature, float& humidity, float& pressure, bool& pressureValid);
    float adjustPressureToSeaLevel(float stationPressureHpa) const;
    float convertPressureToMmHg(float pressureHpa) const;

    RainSensor _rainSensor;
    Sht3xSensor _temperatureSensor;
    Sht3xSensor _humiditySensor;
    Bmp580PressureSensor _pressureSensor;

    unsigned long _lastEnvironmentRead;
    unsigned long _environmentRefreshIntervalMs;
    bool _hasEnvironmentReading;
    float _lastTemperature;
    float _lastHumidity;
    float _lastPressure;
    bool _lastPressureValid;
    float _stationElevationMeters;
    uint8_t _sequence;
};

#endif
