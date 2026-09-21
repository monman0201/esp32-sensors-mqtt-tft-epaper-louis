#include <Wire.h>
#include <SimpleDHT.h>
#include <U8g2lib.h>

const int DHT11_PIN = 14;
const int LIGHT_PIN = 33;
const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;

SimpleDHT11 dht11(DHT11_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

void drawTemperatureIcon(int x, int y) {
  oled.drawRFrame(x + 5, y, 6, 12, 3);
  oled.drawCircle(x + 8, y + 14, 4);
  oled.drawLine(x + 8, y + 4, x + 8, y + 14);
  oled.drawLine(x + 12, y + 4, x + 16, y + 4);
  oled.drawLine(x + 12, y + 8, x + 15, y + 8);
}

void drawHumidityIcon(int x, int y) {
  oled.drawCircle(x + 7, y + 11, 6);
  oled.drawTriangle(x + 7, y, x + 2, y + 8, x + 12, y + 8);
  oled.drawPixel(x + 5, y + 10);
  oled.drawPixel(x + 9, y + 10);
}

void drawBrightnessIcon(int x, int y) {
  oled.drawCircle(x + 6, y + 6, 4);
  oled.drawLine(x + 6, y - 1, x + 6, y + 1);
  oled.drawLine(x + 6, y + 11, x + 6, y + 13);
  oled.drawLine(x - 1, y + 6, x + 1, y + 6);
  oled.drawLine(x + 11, y + 6, x + 13, y + 6);
  oled.drawLine(x + 1, y + 1, x + 3, y + 3);
  oled.drawLine(x + 9, y + 9, x + 11, y + 11);
  oled.drawLine(x + 9, y + 3, x + 11, y + 1);
  oled.drawLine(x + 1, y + 11, x + 3, y + 9);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.begin();
  oled.enableUTF8Print();
  oled.setFont(u8g2_font_wqy12_t_chinese1);
  oled.setFontPosTop();
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  int dhtError = dht11.read(&temperature, &humidity, nullptr);

  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = map(lightRaw, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);

  oled.clearBuffer();

  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(47, 0);
  oled.print("@LOUIS@");
  oled.drawHLine(0, 14, 128);

  oled.drawRFrame(1, 18, 61, 28, 3);
  oled.drawRFrame(66, 18, 61, 28, 3);
  oled.drawVLine(64, 20, 24);

  drawTemperatureIcon(6, 23);
  drawHumidityIcon(72, 23);

  oled.setFont(u8g2_font_7x13B_tf);
  oled.setCursor(27, 23);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(temperature);
  } else {
    oled.print("--");
  }

  oled.setCursor(91, 23);
  if (dhtError == SimpleDHTErrSuccess) {
    oled.print(humidity);
  } else {
    oled.print("--");
  }

  oled.setFont(u8g2_font_wqy12_t_chinese1);
  oled.setCursor(27, 39);
  oled.print("溫度");
  oled.setCursor(91, 39);
  oled.print("濕度");

  drawBrightnessIcon(28, 51);
  oled.setCursor(45, 49);
  oled.print("亮度");

  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(76, 51);
  oled.print(lightPercent);
  oled.print("%");

  oled.sendBuffer();

  Serial.print("Temperature: ");
  if (dhtError == SimpleDHTErrSuccess) {
    Serial.print(temperature);
    Serial.print(", humidity: ");
    Serial.print(humidity);
    Serial.print(" %, ");
  } else {
    Serial.print("ERROR, humidity: ERROR, ");
  }
  Serial.print("Light raw: ");
  Serial.print(lightRaw);
  Serial.print(", brightness: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  delay(1000);
}

