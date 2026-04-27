#ifndef WEATHER_PACKET_CODEC_H
#define WEATHER_PACKET_CODEC_H

#include <Arduino.h>
#include <WeatherPacket.h>

class WeatherPacketCodec {
public:
    static const uint8_t MAGIC_0 = 0x57;
    static const uint8_t MAGIC_1 = 0x53;
    static const size_t PAYLOAD_SIZE = 24;
    static const size_t FRAME_SIZE = 2 + 1 + PAYLOAD_SIZE + 1;

    static size_t frameSize();
    static bool encode(const WeatherPacket& packet, uint8_t* out, size_t outCapacity, size_t& outSize);
    static bool decode(const uint8_t* frame, size_t frameSize, WeatherPacket& packet);

private:
    static uint8_t checksum(const uint8_t* data, size_t length);
    static void writeU16(uint8_t* out, uint16_t value);
    static void writeU32(uint8_t* out, uint32_t value);
    static uint16_t readU16(const uint8_t* in);
    static uint32_t readU32(const uint8_t* in);
};

#endif
