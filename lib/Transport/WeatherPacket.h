#ifndef WEATHER_PACKET_H
#define WEATHER_PACKET_H

#include <Arduino.h>

// Shared sensor payload transported from slave to master.
// packed: ensures identical byte layout on AVR (Nano) and ARM Renesas (UNO R4).
struct __attribute__((packed)) WeatherPacket {
    uint8_t version;
    uint8_t sequence;
    unsigned long sourceTimestampMs;
    float temperature;
    float humidity;
    float pressure;
    uint16_t rainIntensity;
    uint8_t rainLevel;
    bool isRaining;
    bool climateValid;
    bool pressureValid;
};

#endif
