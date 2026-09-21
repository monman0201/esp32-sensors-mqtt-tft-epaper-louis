#include <SPI.h>
#include <SimpleDHT.h>
#include <Adafruit_GFX.h>

// Both libraries provide GFXfont/GFXglyph, but the older ILI9225 library
// uses a misspelled include guard. Load Adafruit_GFX first and suppress the
// duplicate declarations when including the ILI9225 driver.
#define _GFFFONT_H_
#include <TFT_22_ILI9225.h>

// ILI9225 wiring
const int TFT_RST = 26;
const int TFT_RS  = 25;
const int TFT_CS  = 16;
const int TFT_SDI = 23;
const int TFT_CLK = 18;

const int DHT11_PIN = 14;
const int LIGHT_PIN = 33;

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, -1);
SimpleDHT11 dht11(DHT11_PIN);

// Adapter: Adafruit_GFX supplies the drawing API; TFT_22_ILI9225 sends pixels.
class ILI9225_GFX : public Adafruit_GFX {
public:
  ILI9225_GFX() : Adafruit_GFX(176, 220) {}

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x >= 0 && x < 176 && y >= 0 && y < 220) tft.drawPixel(x, y, color);
  }
};

ILI9225_GFX gfx;

const uint16_t BG       = COLOR_BLACK;
const uint16_t PANEL    = 0x18E3;
const uint16_t WHITE_C  = COLOR_WHITE;
const uint16_t TEMP_C   = 0xFD20;
const uint16_t HUMI_C   = 0x07FF;
const uint16_t LIGHT_C  = 0xFFE0;

void drawThermometer(int16_t x, int16_t y, uint16_t color) {
  gfx.drawCircle(x + 8, y + 29, 7, color);
  gfx.fillCircle(x + 8, y + 29, 4, color);
  gfx.drawRoundRect(x + 4, y, 8, 31, 4, color);
  gfx.drawLine(x + 8, y + 7, x + 8, y + 29, color);
}

void drawDrop(int16_t x, int16_t y, uint16_t color) {
  gfx.drawLine(x + 10, y, x, y + 18, color);
  gfx.drawLine(x, y + 18, x + 10, y + 29, color);
  gfx.drawLine(x + 10, y + 29, x + 20, y + 18, color);
  gfx.drawLine(x + 20, y + 18, x + 10, y, color);
  gfx.fillCircle(x + 10, y + 20, 5, color);
}

void drawSun(int16_t x, int16_t y, uint16_t color) {
  gfx.drawCircle(x + 10, y + 15, 7, color);
  gfx.fillCircle(x + 10, y + 15, 4, color);
  for (uint8_t i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    int16_t x1 = x + 10 + (int16_t)(10 * cos(a));
    int16_t y1 = y + 15 + (int16_t)(10 * sin(a));
    int16_t x2 = x + 10 + (int16_t)(14 * cos(a));
    int16_t y2 = y + 15 + (int16_t)(14 * sin(a));
    gfx.drawLine(x1, y1, x2, y2, color);
  }
}

void drawCardFrame(int16_t y, const char* label, uint16_t color, uint8_t icon) {
  gfx.fillRoundRect(4, y, 168, 58, 7, PANEL);
  gfx.drawRoundRect(4, y, 168, 58, 7, color);

  if (icon == 0) drawThermometer(13, y + 13, color);
  if (icon == 1) drawDrop(13, y + 12, color);
  if (icon == 2) drawSun(13, y + 12, color);

  gfx.setTextSize(1);
  gfx.setTextColor(color);
  gfx.setCursor(43, y + 12);
  gfx.print(label);
}

void drawValue(int16_t y, const char* value, uint16_t color, bool alertFlash) {
  // Only this rectangle is updated; the card, icon and label remain untouched.
  gfx.fillRect(40, y + 25, 126, 27, alertFlash ? COLOR_RED : PANEL);
  gfx.setTextSize(2);
  gfx.setTextColor(alertFlash ? WHITE_C : color);
  gfx.setCursor(43, y + 29);
  gfx.print(value);
}

void drawStaticDashboard() {
  tft.setBackgroundColor(BG);
  tft.clear();
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(8, 13);
  gfx.print("ENVIRONMENT MONITOR");

  drawCardFrame(20, "TEMPERATURE", TEMP_C, 0);
  drawCardFrame(82, "HUMIDITY", HUMI_C, 1);
  drawCardFrame(144, "BRIGHTNESS", LIGHT_C, 2);
}

void updateValues(byte temperature, byte humidity, int lightPercent, bool dhtOk, bool alertFlash) {
  char value[16];
  if (dhtOk) {
    snprintf(value, sizeof(value), "%d C", temperature);
    drawValue(20, value, TEMP_C, false);
    snprintf(value, sizeof(value), "%d %%", humidity);
    drawValue(82, value, HUMI_C, humidity > 65 && alertFlash);
  } else {
    drawValue(20, "ERROR", TEMP_C, false);
    drawValue(82, "ERROR", HUMI_C, false);
  }
  snprintf(value, sizeof(value), "%d %%", lightPercent);
  drawValue(144, value, LIGHT_C, false);
}

void setup() {
  Serial.begin(115200);
  SPI.begin(TFT_CLK, -1, TFT_SDI, TFT_CS);
  tft.begin(SPI);
  tft.setOrientation(0);
  drawStaticDashboard();
  Serial.println("ILI9225 Adafruit_GFX dashboard started");
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  bool dhtOk = dht11.read(&temperature, &humidity, nullptr) == SimpleDHTErrSuccess;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);

  bool humidityAlert = dhtOk && humidity > 65;
  for (uint8_t blink = 0; blink < 4; blink++) {
    updateValues(temperature, humidity, lightPercent, dhtOk,
                 humidityAlert && (blink % 2 == 1));
    delay(500);
  }
  Serial.printf("Temperature: %d C, humidity: %d %%, brightness: %d %%\n",
                temperature, humidity, lightPercent);
  delay(2000);
}

