#include "GT24DipReceiver.h"

#include <WeatherPacketCodec.h>

GT24DipReceiver::GT24DipReceiver(HardwareSerial& serial, unsigned long baudRate)
    : _serial(serial), _baudRate(baudRate), _bufferLen(0) {}

bool GT24DipReceiver::begin() {
    _serial.begin(_baudRate);
    _bufferLen = 0;
    return true;
}

bool GT24DipReceiver::receive(WeatherPacket& packet) {
    while (_serial.available() > 0 && _bufferLen < sizeof(_buffer)) {
        _buffer[_bufferLen++] = static_cast<uint8_t>(_serial.read());
    }

    return tryDecodeFromBuffer(packet);
}

bool GT24DipReceiver::tryDecodeFromBuffer(WeatherPacket& packet) {
    const size_t frameSize = WeatherPacketCodec::frameSize();
    if (_bufferLen < frameSize) {
        return false;
    }

    size_t start = 0;
    while (start + frameSize <= _bufferLen) {
        if (_buffer[start] == WeatherPacketCodec::MAGIC_0 && _buffer[start + 1] == WeatherPacketCodec::MAGIC_1) {
            if (WeatherPacketCodec::decode(_buffer + start, frameSize, packet)) {
                size_t consumed = start + frameSize;
                size_t remaining = _bufferLen - consumed;
                if (remaining > 0) {
                    memmove(_buffer, _buffer + consumed, remaining);
                }
                _bufferLen = remaining;
                return true;
            }
        }
        ++start;
    }

    if (_bufferLen > frameSize) {
        size_t keep = frameSize - 1;
        memmove(_buffer, _buffer + (_bufferLen - keep), keep);
        _bufferLen = keep;
    }

    return false;
}
