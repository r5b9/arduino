#ifndef I_RADIO_RECEIVER_H
#define I_RADIO_RECEIVER_H

#include <Arduino.h>
#include <WeatherPacket.h>

class IRadioReceiver {
public:
    virtual ~IRadioReceiver() {}
    virtual bool begin() = 0;
    virtual bool receive(WeatherPacket& packet) = 0;
};

#endif
