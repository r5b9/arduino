#include "CloudPublisher.h"

#include <Logger.h>

CloudPublisher::CloudPublisher(IBackend* backend, unsigned long publishIntervalMs)
    : _backend(backend), _publishIntervalMs(publishIntervalMs), _lastPublishAt(0) {}

bool CloudPublisher::begin() {
    if (!_backend) {
        return false;
    }

    return _backend->begin();
}

bool CloudPublisher::publishIfDue(const WeatherPacket& packet) {
    if (!_backend) {
        return false;
    }

    unsigned long now = millis();
    if ((now - _lastPublishAt) < _publishIntervalMs) {
        return false;
    }

    _lastPublishAt = now;

    WeatherData data;
    data.temperature = packet.climateValid ? packet.temperature : NAN;
    data.humidity = packet.climateValid ? packet.humidity : NAN;
    data.pressure = packet.pressureValid ? packet.pressure : NAN;
    data.rainIntensity = packet.rainIntensity;
    data.isRaining = packet.isRaining;

    if (_backend->sendData(data)) {
        Logger::info("cloud-publisher", "Backend update successful");
        return true;
    }

    Logger::error("cloud-publisher", "Backend update failed");
    return false;
}
