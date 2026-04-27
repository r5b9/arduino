#include "SensorCollector.h"

#include <Logger.h>

SensorCollector::SensorCollector(
    uint8_t rainDigitalPin,
    uint8_t rainAnalogPin,
    uint8_t sht3xAddress,
    uint8_t bmp580Address,
    float stationElevationMeters,
    unsigned long environmentRefreshIntervalMs
) : _rainSensor("RainDigital", rainDigitalPin, "RainAnalog", rainAnalogPin),
    _temperatureSensor("Temperature", sht3xAddress, SHT3X_TEMPERATURE_C),
    _humiditySensor("Humidity", sht3xAddress, SHT3X_HUMIDITY),
    _pressureSensor("Pressure", bmp580Address),
    _lastEnvironmentRead(0),
    _environmentRefreshIntervalMs(environmentRefreshIntervalMs),
    _hasEnvironmentReading(false),
    _lastTemperature(NAN),
    _lastHumidity(NAN),
    _lastPressure(NAN),
    _lastPressureValid(false),
    _stationElevationMeters(stationElevationMeters),
    _sequence(0) {}

bool SensorCollector::begin() {
    bool rainOk = _rainSensor.begin();
    bool temperatureOk = _temperatureSensor.begin();
    bool humidityOk = _humiditySensor.begin();
    bool pressureOk = _pressureSensor.begin();

    if (pressureOk) {
        Logger::info("sensor-collector", "BMP580 pressure sensor initialized");
    } else {
        Logger::warn("sensor-collector", "BMP580 pressure sensor unavailable at startup");
    }

    return rainOk && temperatureOk && humidityOk;
}

WeatherPacket SensorCollector::readPacket() {
    WeatherPacket packet;
    packet.version = 1;
    packet.sequence = _sequence++;
    packet.sourceTimestampMs = millis();

    RainReading rain = _rainSensor.read();
    packet.rainIntensity = rain.intensity;
    packet.rainLevel = static_cast<uint8_t>(rain.level);
    packet.isRaining = rain.isRaining;

    bool pressureValid = false;
    packet.climateValid = readClimate(packet.temperature, packet.humidity, packet.pressure, pressureValid);
    packet.pressureValid = pressureValid;

    return packet;
}

bool SensorCollector::readClimate(float& temperature, float& humidity, float& pressure, bool& pressureValid) {
    unsigned long now = millis();
    if (_hasEnvironmentReading && (now - _lastEnvironmentRead) < _environmentRefreshIntervalMs) {
        temperature = _lastTemperature;
        humidity = _lastHumidity;
        pressure = _lastPressure;
        pressureValid = _lastPressureValid;
        return !isnan(temperature) && !isnan(humidity);
    }

    SensorReading temperatureReading = _temperatureSensor.read();
    SensorReading humidityReading = _humiditySensor.read();
    SensorReading pressureReading = _pressureSensor.read();

    if (!temperatureReading.valid || !humidityReading.valid) {
        if (_hasEnvironmentReading) {
            temperature = _lastTemperature;
            humidity = _lastHumidity;
            pressure = _lastPressure;
            pressureValid = _lastPressureValid;
            Logger::warn("sensor-collector", "SHT3X read failed, reusing last valid climate reading");
            return true;
        }

        temperature = NAN;
        humidity = NAN;
        pressureValid = pressureReading.valid;
        pressure = pressureValid ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
        return false;
    }

    _lastTemperature = temperatureReading.value;
    _lastHumidity = humidityReading.value;
    _lastPressureValid = pressureReading.valid;
    _lastPressure = _lastPressureValid ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
    _lastEnvironmentRead = now;
    _hasEnvironmentReading = true;

    temperature = _lastTemperature;
    humidity = _lastHumidity;
    pressure = _lastPressure;
    pressureValid = _lastPressureValid;

    String environmentLog = "SHT3X ok T=";
    environmentLog += String(temperature, 1);
    environmentLog += "C H=";
    environmentLog += String(humidity, 0);
    environmentLog += "%";
    if (pressureValid) {
        environmentLog += " P0=";
        environmentLog += String(pressure, 1);
        environmentLog += "mmHg";
    } else {
        Logger::warn("sensor-collector", "BMP580 pressure read failed for current sample");
    }
    Logger::info("sensor-collector", environmentLog.c_str());

    return true;
}

float SensorCollector::adjustPressureToSeaLevel(float stationPressureHpa) const {
    if (isnan(stationPressureHpa) || _stationElevationMeters <= 0.0f) {
        return stationPressureHpa;
    }

    return stationPressureHpa / pow(1.0f - (_stationElevationMeters / 44330.0f), 5.255f);
}

float SensorCollector::convertPressureToMmHg(float pressureHpa) const {
    if (isnan(pressureHpa)) {
        return pressureHpa;
    }

    return pressureHpa * 0.75006156f;
}
