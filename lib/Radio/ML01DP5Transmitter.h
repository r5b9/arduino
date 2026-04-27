#ifndef ML01DP5_TRANSMITTER_H
#define ML01DP5_TRANSMITTER_H

#include <Arduino.h>
#include <IRadioTransmitter.h>

// nRF24L01+ transmitter using hardware SPI.
// Default wiring for Nano slave:
//   CE=9, CSN=10, MOSI=D11, MISO=D12, SCK=D13 (standard SPI)
class ML01DP5Transmitter : public IRadioTransmitter {
public:
    struct TxDebugInfo {
        bool          sendOk;
        uint8_t       sequence;
        unsigned long sentAtMs;
        uint8_t       payloadSize;
    };

    ML01DP5Transmitter(uint8_t cePin = 9, uint8_t csnPin = 10);

    bool begin() override;
    bool send(const WeatherPacket& packet) override;
    const TxDebugInfo& lastDebug() const;

private:
    uint8_t     _ce, _csn;
    TxDebugInfo _lastDebug;

    uint8_t spiXfer(uint8_t b);
    uint8_t readReg(uint8_t reg);
    void    writeReg(uint8_t reg, uint8_t val);
    void    writeRegBuf(uint8_t reg, const uint8_t* buf, uint8_t len);
    void    flushTx();
};

#endif
