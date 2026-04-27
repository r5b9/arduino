#ifndef I_RADIO_TRANSMITTER_H
#define I_RADIO_TRANSMITTER_H

#include <Arduino.h>
#include <WeatherPacket.h>

class IRadioTransmitter {
public:
    virtual ~IRadioTransmitter() {}
    virtual bool begin() = 0;
    virtual bool send(const WeatherPacket& packet) = 0;
};

#endif
