#ifndef GT24_DIP_RECEIVER_H
#define GT24_DIP_RECEIVER_H

#include <Arduino.h>
#include <IRadioReceiver.h>

// nRF24L01+ receiver using software SPI.
// Default wiring for UNO R4 WiFi master:
//   CE=5, CSN=11, SCK=13, MOSI=12, MISO=4
class GT24DipReceiver : public IRadioReceiver {
public:
    GT24DipReceiver(uint8_t cePin   = 5,
                    uint8_t csnPin  = 11,
                    uint8_t sckPin  = 13,
                    uint8_t mosiPin = 12,
                    uint8_t misoPin = 4);

    bool begin() override;
    bool receive(WeatherPacket& packet) override;

private:
    uint8_t _ce, _csn, _sck, _mosi, _miso;

    uint8_t spiXfer(uint8_t b);
    uint8_t readReg(uint8_t reg);
    void    writeReg(uint8_t reg, uint8_t val);
    void    writeRegBuf(uint8_t reg, const uint8_t* buf, uint8_t len);
    void    readPayload(uint8_t* buf, uint8_t len);
    void    flushRx();
};

#endif
