#include <Sensors.h>
#include <WeatherPacket.h>
#include <ML01DP5Transmitter.h>

namespace {

// Keep sensor wiring aligned with the legacy UNO implementation.
// Rain digital -> D2, rain analog -> A0, SHT3X -> I2C 0x44, BMP580 -> I2C 0x46(default).
const int rainDigitalPin = 2;
const int rainAnalogPin = A0;
const uint8_t sht3xAddress = 0x44;
const uint8_t bmp580Address = BMP5XX_DEFAULT_ADDRESS;
const float stationElevationMeters = 129.0f;
const unsigned long reportIntervalMs = 2000;
const unsigned long climateRefreshIntervalMs = 5000;
const unsigned long debugBaudRate = 115200;
// nRF24L01 SPI: MOSI=D11, MISO=D12, SCK=D13 (hardware SPI), CE=D9, CSN=D10
const uint8_t radioCePin  = 9;
const uint8_t radioCsnPin = 10;

RainSensor rainSensor("RainDigital", rainDigitalPin, "RainAnalog", rainAnalogPin);
Sht3xSensor temperatureSensor("Temperature", sht3xAddress, SHT3X_TEMPERATURE_C);
Sht3xSensor humiditySensor("Humidity", sht3xAddress, SHT3X_HUMIDITY);
Bmp580PressureSensor pressureSensor("Pressure", bmp580Address);
ML01DP5Transmitter radio(radioCePin, radioCsnPin);
unsigned long lastReportAt = 0;
unsigned long lastClimateAt = 0;
float lastTemperature = NAN;
float lastHumidity = NAN;
float lastPressure = NAN;
bool hasClimate = false;
bool pressureValid = false;
uint8_t sequence = 0;

void debugLog(const __FlashStringHelper* message) {
    Serial.println(message);
}

void debugPacket(const WeatherPacket& packet, const ML01DP5Transmitter::TxDebugInfo& tx) {
    Serial.print(F("seq="));
    Serial.print(packet.sequence);
    Serial.print(F(" ts="));
    Serial.print(packet.sourceTimestampMs);
    Serial.print(F(" climate="));
    Serial.print(packet.climateValid ? F("ok") : F("fail"));
    Serial.print(F(" temp="));
    Serial.print(packet.temperature, 2);
    Serial.print(F(" hum="));
    Serial.print(packet.humidity, 2);
    Serial.print(F(" pressure="));
    Serial.print(packet.pressure, 2);
    Serial.print(F(" pressureValid="));
    Serial.print(packet.pressureValid ? F("yes") : F("no"));
    Serial.print(F(" rainInt="));
    Serial.print(packet.rainIntensity);
    Serial.print(F(" rainState="));
    Serial.print(packet.isRaining ? F("wet") : F("dry"));
    Serial.print(F(" send="));
    Serial.print(tx.sendOk ? F("ok") : F("fail"));
    Serial.print(F(" payload="));
    Serial.println(tx.payloadSize);
}

float adjustPressureToSeaLevel(float stationPressureHpa) {
    if (isnan(stationPressureHpa) || stationElevationMeters <= 0.0f) {
        return stationPressureHpa;
    }

    return stationPressureHpa / pow(1.0f - (stationElevationMeters / 44330.0f), 5.255f);
}

float convertPressureToMmHg(float pressureHpa) {
    if (isnan(pressureHpa)) {
        return pressureHpa;
    }

    return pressureHpa * 0.75006156f;
}

bool readClimate(float& temperature, float& humidity, float& pressure, bool& pressureOk) {
    unsigned long now = millis();
    if (hasClimate && (now - lastClimateAt) < climateRefreshIntervalMs) {
        temperature = lastTemperature;
        humidity = lastHumidity;
        pressure = lastPressure;
        pressureOk = pressureValid;
        debugLog(F("climate cache hit"));
        return true;
    }

    SensorReading temperatureReading = temperatureSensor.read();
    SensorReading humidityReading = humiditySensor.read();
    SensorReading pressureReading = pressureSensor.read();

    if (!temperatureReading.valid || !humidityReading.valid) {
        Serial.print(F("climate read invalid t="));
        Serial.print(temperatureReading.valid ? F("ok") : F("fail"));
        Serial.print(F(" h="));
        Serial.println(humidityReading.valid ? F("ok") : F("fail"));

        if (hasClimate) {
            temperature = lastTemperature;
            humidity = lastHumidity;
            pressure = lastPressure;
            pressureOk = pressureValid;
            debugLog(F("climate fallback to cache"));
            return true;
        }

        temperature = NAN;
        humidity = NAN;
        pressureOk = pressureReading.valid;
        pressure = pressureOk ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
        return false;
    }

    lastTemperature = temperatureReading.value;
    lastHumidity = humidityReading.value;
    pressureValid = pressureReading.valid;
    lastPressure = pressureValid ? convertPressureToMmHg(adjustPressureToSeaLevel(pressureReading.value)) : NAN;
    lastClimateAt = now;
    hasClimate = true;

    Serial.print(F("climate fresh t="));
    Serial.print(lastTemperature, 2);
    Serial.print(F(" h="));
    Serial.print(lastHumidity, 2);
    Serial.print(F(" p="));
    Serial.println(lastPressure, 2);

    temperature = lastTemperature;
    humidity = lastHumidity;
    pressure = lastPressure;
    pressureOk = pressureValid;
    return true;
}

}

void setup() {
    Serial.begin(debugBaudRate);
    debugLog(F("slave boot"));
    Serial.print(F("radio CE="));
    Serial.print(radioCePin);
    Serial.print(F(" CSN="));
    Serial.println(radioCsnPin);

    // Temporary: disable rain sensor hardware invocation while keeping implementation in code.
    // rainSensor.begin();
    temperatureSensor.begin();
    humiditySensor.begin();
    pressureSensor.begin();
    radio.begin();

    debugLog(F("sensors initialized"));
    debugLog(F("radio initialized"));
}

void loop() {
    unsigned long now = millis();
    if ((now - lastReportAt) < reportIntervalMs) {
        return;
    }

    lastReportAt = now;

    // Temporary: disable rain sensor hardware invocation while keeping implementation in code.
    // RainReading rain = rainSensor.read();

    float temperature = NAN;
    float humidity = NAN;
    float pressure = NAN;
    bool currentPressureValid = false;
    bool climateValid = readClimate(temperature, humidity, pressure, currentPressureValid);

    WeatherPacket packet;
    packet.version = 1;
    packet.sequence = sequence++;
    packet.sourceTimestampMs = now;
    packet.temperature = temperature;
    packet.humidity = humidity;
    packet.pressure = pressure;
    packet.rainIntensity = 0;
    packet.rainLevel = static_cast<uint8_t>(RAIN_NONE);
    packet.isRaining = false;
    packet.climateValid = climateValid;
    packet.pressureValid = currentPressureValid;

    radio.send(packet);
    debugPacket(packet, radio.lastDebug());
}
