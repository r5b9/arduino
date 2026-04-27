// GMG12864-06D display diagnostic - auto-cycles through controller variants
// Wiring: CS=10, RSE=9, RS=8, SCL=7, SI=6
// U8g2 4W SW SPI order: (rotation, clk, data, cs, dc, reset)
//                                   7     6    10   8    9

#include <U8g2lib.h>

#define CLK  7
#define DAT  6
#define CS   10
#define DC   8
#define RST  9

// Candidate drivers for common 128x64 SPI LCD modules
U8G2_ST7565_ERC12864_F_4W_SW_SPI      drv0(U8G2_R0, CLK, DAT, CS, DC, RST);
U8G2_ST7565_NHD_C12864_F_4W_SW_SPI   drv1(U8G2_R0, CLK, DAT, CS, DC, RST);
U8G2_ST7565_JLX12864_F_4W_SW_SPI     drv2(U8G2_R0, CLK, DAT, CS, DC, RST);
U8G2_UC1701_MINI12864_F_4W_SW_SPI    drv3(U8G2_R0, CLK, DAT, CS, DC, RST);
U8G2_UC1701_EA_DOGS102_F_4W_SW_SPI   drv4(U8G2_R0, CLK, DAT, CS, DC, RST);
U8G2_ST7567_JLX12864_F_4W_SW_SPI     drv5(U8G2_R0, CLK, DAT, CS, DC, RST);
U8G2_ST7567_ENH_DG128064_F_4W_SW_SPI drv6(U8G2_R0, CLK, DAT, CS, DC, RST);

const char* names[] = {
    "ST7565_ERC12864",
    "ST7565_NHD_C12864",
    "ST7565_JLX12864",
    "UC1701_MINI12864",
    "UC1701_EA_DOGS102",
    "ST7567_JLX12864",
    "ST7567_ENH_DG128064",
};

U8G2* drivers[] = { &drv0, &drv1, &drv2, &drv3, &drv4, &drv5, &drv6 };
const uint8_t NUM_DRIVERS = sizeof(drivers) / sizeof(drivers[0]);

uint8_t current = 0;
unsigned long lastSwitch = 0;
const unsigned long switchIntervalMs = 10000;

void testDriver(U8G2* d, const char* name) {
    d->begin();
    d->setContrast(220);
    d->clearBuffer();
    d->setFont(u8g2_font_logisoso28_tr);
    char id[8];
    snprintf(id, sizeof(id), "#%u", current);
    d->drawStr(0, 34, id);

    d->setFont(u8g2_font_6x10_tf);
    d->drawStr(0, 48, name);
    d->drawStr(0, 60, "10s per driver");
    d->sendBuffer();
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("=== Display driver scan ===");
    Serial.print("Testing driver 0: ");
    Serial.println(names[0]);
    testDriver(drivers[0], names[0]);
    lastSwitch = millis();
}

void loop() {
    if (millis() - lastSwitch > switchIntervalMs) {
        current = (current + 1) % NUM_DRIVERS;
        lastSwitch = millis();
        Serial.print("Testing driver ");
        Serial.print(current);
        Serial.print(": ");
        Serial.println(names[current]);
        testDriver(drivers[current], names[current]);
    }
}

