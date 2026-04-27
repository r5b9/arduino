#include "ML01DP5Transmitter.h"

#include <WeatherPacketCodec.h>
#include <string.h>

ML01DP5Transmitter::ML01DP5Transmitter(HardwareSerial& serial, unsigned long baudRate)
    : _hardwareSerial(&serial), _softwareSerial(nullptr), _stream(&serial), _baudRate(baudRate), _lastDebug() {
    memset(&_lastDebug, 0, sizeof(_lastDebug));
}

ML01DP5Transmitter::ML01DP5Transmitter(SoftwareSerial& serial, unsigned long baudRate)
    : _hardwareSerial(nullptr), _softwareSerial(&serial), _stream(&serial), _baudRate(baudRate), _lastDebug() {
    memset(&_lastDebug, 0, sizeof(_lastDebug));
}

bool ML01DP5Transmitter::begin() {
    if (_hardwareSerial) {
        _hardwareSerial->begin(_baudRate);
    }

    if (_softwareSerial) {
        _softwareSerial->begin(_baudRate);
    }

    return true;
}

bool ML01DP5Transmitter::send(const WeatherPacket& packet) {
    _lastDebug.encodeOk = false;
    _lastDebug.sendOk = false;
    _lastDebug.sequence = packet.sequence;
    _lastDebug.sentAtMs = millis();
    _lastDebug.frameSize = 0;
    _lastDebug.written = 0;
    _lastDebug.checksum = 0;

    uint8_t frame[WeatherPacketCodec::FRAME_SIZE];
    size_t frameSize = 0;
    if (!WeatherPacketCodec::encode(packet, frame, sizeof(frame), frameSize)) {
        return false;
    }

    _lastDebug.encodeOk = true;
    _lastDebug.frameSize = frameSize;
    _lastDebug.checksum = frame[frameSize - 1];
    if (frameSize <= sizeof(_lastDebug.frame)) {
        memcpy(_lastDebug.frame, frame, frameSize);
    }

    size_t written = _stream->write(frame, frameSize);
    _lastDebug.written = written;
    _lastDebug.sendOk = written == frameSize;
    return _lastDebug.sendOk;
}

const ML01DP5Transmitter::TxDebugInfo& ML01DP5Transmitter::lastDebug() const {
    return _lastDebug;
}
