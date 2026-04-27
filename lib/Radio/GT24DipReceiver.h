#ifndef GT24_DIP_RECEIVER_H
#define GT24_DIP_RECEIVER_H

#include <Arduino.h>
#include <IRadioReceiver.h>

class GT24DipReceiver : public IRadioReceiver {
public:
    GT24DipReceiver(HardwareSerial& serial, unsigned long baudRate = 57600);

    bool begin() override;
    bool receive(WeatherPacket& packet) override;

private:
    HardwareSerial& _serial;
    unsigned long _baudRate;
    uint8_t _buffer[64];
    size_t _bufferLen;

    bool tryDecodeFromBuffer(WeatherPacket& packet);
};

#endif
