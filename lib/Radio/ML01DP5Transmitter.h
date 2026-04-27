#ifndef ML01DP5_TRANSMITTER_H
#define ML01DP5_TRANSMITTER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <IRadioTransmitter.h>

class ML01DP5Transmitter : public IRadioTransmitter {
public:
    struct TxDebugInfo {
        bool encodeOk;
        bool sendOk;
        uint8_t sequence;
        unsigned long sentAtMs;
        size_t frameSize;
        size_t written;
        uint8_t checksum;
        uint8_t frame[64];
    };

    ML01DP5Transmitter(HardwareSerial& serial, unsigned long baudRate = 57600);
    ML01DP5Transmitter(SoftwareSerial& serial, unsigned long baudRate = 57600);

    bool begin() override;
    bool send(const WeatherPacket& packet) override;
    const TxDebugInfo& lastDebug() const;

private:
    HardwareSerial* _hardwareSerial;
    SoftwareSerial* _softwareSerial;
    Stream* _stream;
    unsigned long _baudRate;
    TxDebugInfo _lastDebug;
};

#endif
