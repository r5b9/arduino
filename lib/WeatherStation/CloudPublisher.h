#ifndef CLOUD_PUBLISHER_H
#define CLOUD_PUBLISHER_H

#include <Arduino.h>
#include <IBackend.h>
#include <WeatherPacket.h>

class CloudPublisher {
public:
    CloudPublisher(IBackend* backend = nullptr, unsigned long publishIntervalMs = 15000);

    bool begin();
    bool publishIfDue(const WeatherPacket& packet);

private:
    IBackend* _backend;
    unsigned long _publishIntervalMs;
    unsigned long _lastPublishAt;
};

#endif
