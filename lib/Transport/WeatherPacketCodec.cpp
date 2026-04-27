#include "WeatherPacketCodec.h"

#include <string.h>

size_t WeatherPacketCodec::frameSize() {
    return FRAME_SIZE;
}

bool WeatherPacketCodec::encode(const WeatherPacket& packet, uint8_t* out, size_t outCapacity, size_t& outSize) {
    if (!out || outCapacity < FRAME_SIZE) {
        outSize = 0;
        return false;
    }

    out[0] = MAGIC_0;
    out[1] = MAGIC_1;
    out[2] = static_cast<uint8_t>(PAYLOAD_SIZE);

    uint8_t* payload = out + 3;
    payload[0] = packet.version;
    payload[1] = packet.sequence;
    writeU32(payload + 2, static_cast<uint32_t>(packet.sourceTimestampMs));

    memcpy(payload + 6, &packet.temperature, sizeof(float));
    memcpy(payload + 10, &packet.humidity, sizeof(float));
    memcpy(payload + 14, &packet.pressure, sizeof(float));

    writeU16(payload + 18, packet.rainIntensity);
    payload[20] = packet.rainLevel;
    payload[21] = packet.isRaining ? 1 : 0;
    payload[22] = packet.climateValid ? 1 : 0;
    payload[23] = packet.pressureValid ? 1 : 0;

    out[FRAME_SIZE - 1] = checksum(out + 2, 1 + PAYLOAD_SIZE);

    outSize = FRAME_SIZE;
    return true;
}

bool WeatherPacketCodec::decode(const uint8_t* frame, size_t size, WeatherPacket& packet) {
    if (!frame || size != FRAME_SIZE) {
        return false;
    }

    if (frame[0] != MAGIC_0 || frame[1] != MAGIC_1) {
        return false;
    }

    if (frame[2] != PAYLOAD_SIZE) {
        return false;
    }

    uint8_t expectedChecksum = checksum(frame + 2, 1 + PAYLOAD_SIZE);
    if (frame[FRAME_SIZE - 1] != expectedChecksum) {
        return false;
    }

    const uint8_t* payload = frame + 3;
    packet.version = payload[0];
    packet.sequence = payload[1];
    packet.sourceTimestampMs = readU32(payload + 2);

    memcpy(&packet.temperature, payload + 6, sizeof(float));
    memcpy(&packet.humidity, payload + 10, sizeof(float));
    memcpy(&packet.pressure, payload + 14, sizeof(float));

    packet.rainIntensity = readU16(payload + 18);
    packet.rainLevel = payload[20];
    packet.isRaining = payload[21] != 0;
    packet.climateValid = payload[22] != 0;
    packet.pressureValid = payload[23] != 0;

    return true;
}

uint8_t WeatherPacketCodec::checksum(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
    }
    return crc;
}

void WeatherPacketCodec::writeU16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
}

void WeatherPacketCodec::writeU32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFu);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

uint16_t WeatherPacketCodec::readU16(const uint8_t* in) {
    return static_cast<uint16_t>(in[0]) | (static_cast<uint16_t>(in[1]) << 8);
}

uint32_t WeatherPacketCodec::readU32(const uint8_t* in) {
    return static_cast<uint32_t>(in[0]) |
        (static_cast<uint32_t>(in[1]) << 8) |
        (static_cast<uint32_t>(in[2]) << 16) |
        (static_cast<uint32_t>(in[3]) << 24);
}
