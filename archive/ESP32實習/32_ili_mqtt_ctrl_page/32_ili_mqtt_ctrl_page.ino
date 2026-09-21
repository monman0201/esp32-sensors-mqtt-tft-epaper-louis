#include <SPI.h>
#include <time.h>
#include <WiFi.h>
#include <SimpleDHT.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>

// Adafruit_GFX and the older ILI9225 library both declare GFXfont/GFXglyph.
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
const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;
const int PAGE_BUTTON_PIN = 0;

// WiFi / MQTT
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_DATA_TOPIC = "louis/class305/data";
const char* MQTT_GLED_TOPIC = "louis/class305/ctrl/gled";
const char* MQTT_YLED_TOPIC = "louis/class305/ctrl/yled";
const char* MQTT_RLED_TOPIC = "louis/class305/ctrl/rled";
const unsigned long MQTT_DATA_INTERVAL = 10000UL;
const long NTP_GMT_OFFSET_SEC = 8L * 60L * 60L;
const int NTP_DAYLIGHT_OFFSET_SEC = 0;
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.nist.gov";

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, -1);
SimpleDHT11 dht11(DHT11_PIN);
WiFiClient mqttClient;

class ILI9225_GFX : public Adafruit_GFX {
public:
  ILI9225_GFX() : Adafruit_GFX(176, 220) {}

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x >= 0 && x < 176 && y >= 0 && y < 220) {
      tft.drawPixel(x, y, color);
    }
  }
};

ILI9225_GFX gfx;

const uint16_t BG       = COLOR_BLACK;
const uint16_t PANEL    = 0x18E3;
const uint16_t WHITE_C  = COLOR_WHITE;
const uint16_t TEMP_C   = 0xFD20;
const uint16_t HUMI_C   = 0x07FF;
const uint16_t LIGHT_C  = 0xFFE0;
const uint16_t STATUS_C = 0x0841;
const uint16_t GRID_C   = 0x39E7;
const uint16_t GREEN_C  = COLOR_GREEN;
const uint16_t RED_C    = COLOR_RED;

unsigned long lastDataPublish = 0;
unsigned long lastPing = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastTemperatureSample = 0;
unsigned long lastPageButtonChange = 0;
unsigned long lastDetailPageDraw = 0;
const unsigned long TEMPERATURE_SAMPLE_INTERVAL = 10000UL;
const unsigned long BUTTON_DEBOUNCE_INTERVAL = 50UL;

volatile bool pageButtonPressed = false;
uint8_t currentPage = 0;
const uint8_t TEMPERATURE_HISTORY_SIZE = 60;
int16_t temperatureHistory[TEMPERATURE_HISTORY_SIZE];
uint8_t temperatureHistoryIndex = 0;
uint8_t temperatureHistoryCount = 0;
byte latestTemperature = 0;
bool latestTemperatureValid = false;
byte latestHumidity = 0;
bool latestHumidityValid = false;
int latestLightPercent = 0;
int16_t humidityHistory[TEMPERATURE_HISTORY_SIZE];
uint8_t humidityHistoryIndex = 0;
uint8_t humidityHistoryCount = 0;
int16_t lightHistory[TEMPERATURE_HISTORY_SIZE];
uint8_t lightHistoryIndex = 0;
uint8_t lightHistoryCount = 0;
bool greenLedOn = false;
bool yellowLedOn = false;
bool redLedOn = false;

void writeMqttString(const String& value) {
  mqttClient.write((const uint8_t*)value.c_str(), value.length());
}

void writeMqttLength(uint32_t length) {
  do {
    uint8_t encoded = length % 128;
    length /= 128;
    if (length > 0) encoded |= 0x80;
    mqttClient.write(encoded);
  } while (length > 0);
}

void updateLeds() {
  digitalWrite(GREEN_LED_PIN, greenLedOn ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellowLedOn ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, redLedOn ? HIGH : LOW);
}

void mqttCallback(const char* topic, const String& payload) {
  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return;
  }

  bool changed = false;
  String receivedTopic = topic;
  if (receivedTopic == MQTT_GLED_TOPIC && !document["gled"].isNull()) {
    String value = document["gled"].as<String>();
    if (value == "on" || value == "off") {
      greenLedOn = value == "on";
      changed = true;
    }
  }
  if (receivedTopic == MQTT_YLED_TOPIC && !document["yled"].isNull()) {
    String value = document["yled"].as<String>();
    if (value == "on" || value == "off") {
      yellowLedOn = value == "on";
      changed = true;
    }
  }
  if (receivedTopic == MQTT_RLED_TOPIC && !document["rled"].isNull()) {
    String value = document["rled"].as<String>();
    if (value == "on" || value == "off") {
      redLedOn = value == "on";
      changed = true;
    }
  }

  if (changed) {
    updateLeds();
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(payload);
    Serial.printf("LED state: green=%s, yellow=%s, red=%s\n",
                  greenLedOn ? "on" : "off",
                  yellowLedOn ? "on" : "off",
                  redLedOn ? "on" : "off");
  }
}

String makeClientId() {
  return "esp32-ili-" + String((uint32_t)esp_random(), HEX);
}

void drawStatusBar() {
  gfx.fillRect(0, 0, 176, 16, STATUS_C);
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(5, 5);
  gfx.print("WIFI:");
  gfx.setTextColor(WiFi.status() == WL_CONNECTED ? GREEN_C : RED_C);
  gfx.print(WiFi.status() == WL_CONNECTED ? "O" : "X");
  gfx.setTextColor(WHITE_C);
  gfx.print("  MQTT:");
  gfx.setTextColor(mqttClient.connected() ? GREEN_C : RED_C);
  gfx.print(mqttClient.connected() ? "O" : "X");
  gfx.setTextColor(COLOR_CYAN);
  gfx.setCursor(146, 5);
  gfx.print("P");
  gfx.print(currentPage + 1);
  gfx.print("/4");
}

void drawConnectionMessage(const char* title, const char* detail) {
  tft.setBackgroundColor(BG);
  tft.clear();
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(8, 25);
  gfx.print(title);
  gfx.setTextColor(COLOR_CYAN);
  gfx.setCursor(8, 45);
  gfx.print(detail);
  drawStatusBar();
}

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
  gfx.fillRoundRect(4, y, 168, 46, 7, PANEL);
  gfx.drawRoundRect(4, y, 168, 46, 7, color);
  if (icon == 0) drawThermometer(13, y + 7, color);
  if (icon == 1) drawDrop(13, y + 5, color);
  if (icon == 2) drawSun(13, y + 5, color);
  gfx.setTextSize(1);
  gfx.setTextColor(color);
  gfx.setCursor(43, y + 8);
  gfx.print(label);
}

void drawValue(int16_t y, const char* value, uint16_t color, bool alertFlash) {
  gfx.fillRect(40, y + 20, 126, 22, alertFlash ? RED_C : PANEL);
  gfx.setTextSize(2);
  gfx.setTextColor(alertFlash ? WHITE_C : color);
  gfx.setCursor(43, y + 23);
  gfx.print(value);
}

void drawDateTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 100)) {
    gfx.fillRect(0, 186, 176, 34, BG);
    gfx.setTextSize(1);
    gfx.setTextColor(WHITE_C);
    gfx.setCursor(38, 204);
    gfx.print("--/-- --- --:--");
    return;
  }

  char dateTime[24];
  strftime(dateTime, sizeof(dateTime), "%m/%d %a %H:%M", &timeInfo);
  gfx.fillRect(0, 186, 176, 34, BG);
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(38, 204);
  gfx.print(dateTime);
}

void recordHistoryValue(int16_t value, int16_t* history,
                        uint8_t& historyIndex, uint8_t& historyCount) {
  history[historyIndex] = value;
  historyIndex = (historyIndex + 1) % TEMPERATURE_HISTORY_SIZE;
  if (historyCount < TEMPERATURE_HISTORY_SIZE) {
    historyCount++;
  }
}

void recordTemperature(byte temperature) {
  recordHistoryValue(temperature, temperatureHistory,
                     temperatureHistoryIndex, temperatureHistoryCount);
}

void recordHumidity(byte humidity) {
  recordHistoryValue(humidity, humidityHistory,
                     humidityHistoryIndex, humidityHistoryCount);
}

void recordLight(int lightPercent) {
  recordHistoryValue(lightPercent, lightHistory,
                     lightHistoryIndex, lightHistoryCount);
}

void drawGaugeArc(int16_t centerX, int16_t centerY,
                  int16_t innerRadius, int16_t outerRadius,
                  int16_t startAngle, int16_t endAngle, uint16_t color) {
  for (int16_t angle = startAngle; angle <= endAngle; angle++) {
    float radians = angle * 0.0174532925f;
    int16_t x1 = centerX + (int16_t)(innerRadius * cos(radians));
    int16_t y1 = centerY + (int16_t)(innerRadius * sin(radians));
    int16_t x2 = centerX + (int16_t)(outerRadius * cos(radians));
    int16_t y2 = centerY + (int16_t)(outerRadius * sin(radians));
    gfx.drawLine(x1, y1, x2, y2, color);
  }
}

void drawTemperatureGauge() {
  const int16_t centerX = 88;
  const int16_t centerY = 94;
  const int16_t innerRadius = 49;
  const int16_t outerRadius = 62;

  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(43, 21);
  gfx.print("CURRENT TEMP");

  drawGaugeArc(centerX, centerY, innerRadius, outerRadius, 180, 239, GREEN_C);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius, 241, 299, LIGHT_C);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius, 301, 360, RED_C);

  // White separators between the three gauge ranges.
  drawGaugeArc(centerX, centerY, innerRadius - 2, outerRadius + 2,
               240, 240, WHITE_C);
  drawGaugeArc(centerX, centerY, innerRadius - 2, outerRadius + 2,
               300, 300, WHITE_C);

  gfx.setTextColor(GREEN_C);
  gfx.setCursor(14, 112);
  gfx.print("10");
  gfx.setTextColor(LIGHT_C);
  gfx.setCursor(49, 31);
  gfx.print("20");
  gfx.setTextColor(RED_C);
  gfx.setCursor(119, 31);
  gfx.print("30");
  gfx.setCursor(153, 112);
  gfx.print("40");

  if (temperatureHistoryCount == 0) {
    gfx.setTextColor(WHITE_C);
    gfx.setCursor(76, 105);
    gfx.print("--");
    return;
  }

  uint8_t newestIndex = (temperatureHistoryIndex +
                         TEMPERATURE_HISTORY_SIZE - 1) %
                        TEMPERATURE_HISTORY_SIZE;
  int16_t temperature = constrain(temperatureHistory[newestIndex], 10, 40);
  int16_t needleAngle = 180 + (temperature - 10) * 180 / 30;
  float radians = needleAngle * 0.0174532925f;
  int16_t needleX = centerX + (int16_t)(45 * cos(radians));
  int16_t needleY = centerY + (int16_t)(45 * sin(radians));
  gfx.drawLine(centerX, centerY, needleX, needleY, WHITE_C);
  gfx.fillCircle(centerX, centerY, 4, WHITE_C);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(73, 105);
  gfx.print(temperature);
  gfx.print(" C");
}

void drawTemperatureChart() {
  const int16_t chartLeft = 29;
  const int16_t chartRight = 171;
  const int16_t chartTop = 126;
  const int16_t chartBottom = 204;

  tft.setBackgroundColor(BG);
  tft.clear();
  drawStatusBar();
  drawTemperatureGauge();
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(62, 118);
  gfx.print("TEMP TREND");

  for (int temperature = 10; temperature <= 40; temperature += 5) {
    int16_t y = chartBottom - (temperature - 10) *
                (chartBottom - chartTop) / 30;
    gfx.drawLine(chartLeft, y, chartRight, y, GRID_C);
    gfx.setCursor(2, y - 3);
    gfx.print(temperature);
  }
  gfx.drawLine(chartLeft, chartTop, chartLeft, chartBottom, WHITE_C);
  gfx.drawLine(chartLeft, chartBottom, chartRight, chartBottom, WHITE_C);
  gfx.setCursor(28, 216);
  gfx.print("-10m");
  gfx.setCursor(150, 216);
  gfx.print("NOW");

  if (temperatureHistoryCount < 2) {
    gfx.setTextColor(COLOR_CYAN);
    gfx.setCursor(70, 170);
    gfx.print("DATA ");
    gfx.print(temperatureHistoryCount);
    gfx.print("/60");
    if (temperatureHistoryCount == 0) return;
  }

  uint8_t firstIndex = temperatureHistoryCount == TEMPERATURE_HISTORY_SIZE
                           ? temperatureHistoryIndex : 0;
  int16_t previousX = 0;
  int16_t previousY = 0;
  for (uint8_t point = 0; point < temperatureHistoryCount; point++) {
    uint8_t index = (firstIndex + point) % TEMPERATURE_HISTORY_SIZE;
    int16_t value = constrain(temperatureHistory[index], 10, 40);
    int16_t x = chartRight -
                (int16_t)((temperatureHistoryCount - 1 - point) *
                          (chartRight - chartLeft) / 59.0f);
    int16_t y = chartBottom - (value - 10) *
                (chartBottom - chartTop) / 30;
    if (point > 0) {
      gfx.drawLine(previousX, previousY, x, y, TEMP_C);
    }
    // Draw the thin connection first so each temperature sample remains a clear dot.
    gfx.fillCircle(x, y, 3, TEMP_C);
    previousX = x;
    previousY = y;
  }
}

void drawTemperatureDetail() {
  tft.setBackgroundColor(BG);
  tft.clear();
  drawStatusBar();
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(45, 22);
  gfx.print("TEMPERATURE");

  if (!latestTemperatureValid) {
    gfx.setTextSize(3);
    gfx.setCursor(62, 78);
    gfx.print("ERR");
    return;
  }

  int16_t temperature = constrain(latestTemperature, 10, 40);
  gfx.setTextSize(3);
  gfx.setTextColor(TEMP_C);
  gfx.setCursor(52, 78);
  gfx.print(latestTemperature);
  gfx.print(" C");

  drawGaugeArc(88, 158, 48, 60, 180, 239, GREEN_C);
  drawGaugeArc(88, 158, 48, 60, 241, 299, LIGHT_C);
  drawGaugeArc(88, 158, 48, 60, 301, 360, RED_C);
  drawGaugeArc(88, 158, 46, 62, 240, 240, WHITE_C);
  drawGaugeArc(88, 158, 46, 62, 300, 300, WHITE_C);

  int16_t needleAngle = 180 + (temperature - 10) * 180 / 30;
  float radians = needleAngle * 0.0174532925f;
  gfx.drawLine(88, 158,
               88 + (int16_t)(43 * cos(radians)),
               158 + (int16_t)(43 * sin(radians)), WHITE_C);
  gfx.fillCircle(88, 158, 4, WHITE_C);
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(18, 181);
  gfx.print("10");
  gfx.setCursor(153, 181);
  gfx.print("40 C");
}

void drawBrightnessDetail() {
  tft.setBackgroundColor(BG);
  tft.clear();
  drawStatusBar();
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(43, 22);
  gfx.print("BRIGHTNESS");

  gfx.setTextSize(3);
  gfx.setTextColor(LIGHT_C);
  gfx.setCursor(48, 78);
  gfx.print(latestLightPercent);
  gfx.print("%");

  const int16_t barLeft = 14;
  const int16_t barTop = 111;
  const int16_t barWidth = 148;
  const int16_t barHeight = 25;
  gfx.drawRoundRect(barLeft, barTop, barWidth, barHeight, 5, WHITE_C);
  int16_t fillWidth = map(constrain(latestLightPercent, 0, 100),
                          0, 100, 0, barWidth - 4);
  if (fillWidth > 0) {
    gfx.fillRoundRect(barLeft + 2, barTop + 2, fillWidth, barHeight - 4,
                      3, LIGHT_C);
  }
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(12, 153);
  gfx.print("0%");
  gfx.setCursor(153, 153);
  gfx.print("100%");
  gfx.setCursor(42, 190);
  gfx.print("LIGHT SENSOR GPIO33");
}

void drawMetricPage(const char* title, const char* unit,
                    int16_t latestValue, bool valueValid,
                    int16_t* history, uint8_t historyCount,
                    uint8_t historyIndex, int16_t minimum,
                    int16_t maximum, int16_t boundaryOne,
                    int16_t boundaryTwo, uint16_t colorOne,
                    uint16_t colorTwo, uint16_t colorThree,
                    int16_t tickStep) {
  const int16_t centerX = 88;
  const int16_t centerY = 94;
  const int16_t innerRadius = 49;
  const int16_t outerRadius = 62;
  const int16_t chartLeft = 29;
  const int16_t chartRight = 171;
  const int16_t chartTop = 126;
  const int16_t chartBottom = 204;

  tft.setBackgroundColor(BG);
  tft.clear();
  drawStatusBar();

  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(49, 21);
  gfx.print(title);

  int16_t angleOne = 180 + (boundaryOne - minimum) * 180 /
                     (maximum - minimum);
  int16_t angleTwo = 180 + (boundaryTwo - minimum) * 180 /
                     (maximum - minimum);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius,
               180, angleOne - 1, colorOne);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius,
               angleOne + 1, angleTwo - 1, colorTwo);
  drawGaugeArc(centerX, centerY, innerRadius, outerRadius,
               angleTwo + 1, 360, colorThree);
  drawGaugeArc(centerX, centerY, innerRadius - 2, outerRadius + 2,
               angleOne, angleOne, WHITE_C);
  drawGaugeArc(centerX, centerY, innerRadius - 2, outerRadius + 2,
               angleTwo, angleTwo, WHITE_C);

  gfx.setTextColor(colorOne);
  gfx.setCursor(12, 112);
  gfx.print(minimum);
  gfx.setTextColor(colorTwo);
  gfx.setCursor(49, 31);
  gfx.print(boundaryOne);
  gfx.setTextColor(colorThree);
  gfx.setCursor(119, 31);
  gfx.print(boundaryTwo);
  gfx.setCursor(150, 112);
  gfx.print(maximum);

  if (valueValid) {
    int16_t gaugeValue = constrain(latestValue, minimum, maximum);
    int16_t needleAngle = 180 + (gaugeValue - minimum) * 180 /
                          (maximum - minimum);
    float radians = needleAngle * 0.0174532925f;
    gfx.drawLine(centerX, centerY,
                 centerX + (int16_t)(45 * cos(radians)),
                 centerY + (int16_t)(45 * sin(radians)), WHITE_C);
    gfx.fillCircle(centerX, centerY, 4, WHITE_C);
    gfx.setTextColor(WHITE_C);
    gfx.setCursor(70, 105);
    gfx.print(latestValue);
    gfx.print(unit);
  } else {
    gfx.setTextColor(WHITE_C);
    gfx.setCursor(76, 105);
    gfx.print("--");
  }

  gfx.setTextColor(WHITE_C);
  gfx.setCursor(62, 118);
  gfx.print(title);
  gfx.print(" TREND");
  for (int16_t value = minimum; value <= maximum; value += tickStep) {
    int16_t y = chartBottom - (value - minimum) *
                (chartBottom - chartTop) / (maximum - minimum);
    gfx.drawLine(chartLeft, y, chartRight, y, GRID_C);
    gfx.setCursor(0, y - 3);
    gfx.print(value);
  }
  gfx.drawLine(chartLeft, chartTop, chartLeft, chartBottom, WHITE_C);
  gfx.drawLine(chartLeft, chartBottom, chartRight, chartBottom, WHITE_C);
  gfx.setCursor(28, 216);
  gfx.print("-10m");
  gfx.setCursor(150, 216);
  gfx.print("NOW");

  if (historyCount < 2) {
    gfx.setTextColor(COLOR_CYAN);
    gfx.setCursor(70, 170);
    gfx.print("DATA ");
    gfx.print(historyCount);
    gfx.print("/60");
    if (historyCount == 0) return;
  }

  uint8_t firstIndex = historyCount == TEMPERATURE_HISTORY_SIZE
                           ? historyIndex : 0;
  int16_t previousX = 0;
  int16_t previousY = 0;
  for (uint8_t point = 0; point < historyCount; point++) {
    uint8_t index = (firstIndex + point) % TEMPERATURE_HISTORY_SIZE;
    int16_t value = constrain(history[index], minimum, maximum);
    int16_t x = chartRight -
                (int16_t)((historyCount - 1 - point) *
                          (chartRight - chartLeft) / 59.0f);
    int16_t y = chartBottom - (value - minimum) *
                (chartBottom - chartTop) / (maximum - minimum);
    if (point > 0) {
      gfx.drawLine(previousX, previousY, x, y, colorTwo);
    }
    gfx.fillCircle(x, y, 3, colorTwo);
    previousX = x;
    previousY = y;
  }
}

void drawCurrentPage() {
  if (currentPage == 0) {
    drawStaticDashboard();
  } else if (currentPage == 1) {
    drawTemperatureChart();
  } else if (currentPage == 2) {
    drawMetricPage("HUMIDITY", "%", latestHumidity,
                   latestHumidityValid, humidityHistory,
                   humidityHistoryCount, humidityHistoryIndex,
                   0, 100, 40, 70, LIGHT_C, GREEN_C, RED_C, 20);
  } else {
    drawMetricPage("BRIGHTNESS", "%", latestLightPercent, true,
                   lightHistory, lightHistoryCount, lightHistoryIndex,
                   0, 100, 30, 70, 0x001F, LIGHT_C, RED_C, 20);
  }
}

void IRAM_ATTR onPageButtonPressed() {
  pageButtonPressed = true;
}

void handlePageButton() {
  bool pressed = false;
  noInterrupts();
  if (pageButtonPressed) {
    pageButtonPressed = false;
    pressed = true;
  }
  interrupts();

  if (pressed && millis() - lastPageButtonChange >= BUTTON_DEBOUNCE_INTERVAL) {
    lastPageButtonChange = millis();
      currentPage = (currentPage + 1) % 4;
    Serial.printf("Page changed to: %u\n", currentPage + 1);
    drawCurrentPage();
  }
}

void drawStaticDashboard() {
  tft.setBackgroundColor(BG);
  tft.clear();
  drawStatusBar();
  gfx.setTextSize(1);
  gfx.setTextColor(WHITE_C);
  gfx.setCursor(8, 24);
  gfx.print("ENVIRONMENT MONITOR");
  drawCardFrame(30, "TEMPERATURE", TEMP_C, 0);
  drawCardFrame(82, "HUMIDITY", HUMI_C, 1);
  drawCardFrame(134, "BRIGHTNESS", LIGHT_C, 2);
  drawDateTime();
}

void updateValues(byte temperature, byte humidity, int lightPercent,
                  bool dhtOk, bool alertFlash) {
  char value[16];
  if (dhtOk) {
    snprintf(value, sizeof(value), "%d C", temperature);
    drawValue(30, value, TEMP_C, false);
    snprintf(value, sizeof(value), "%d %%", humidity);
    drawValue(82, value, HUMI_C, humidity > 65 && alertFlash);
  } else {
    drawValue(30, "ERROR", TEMP_C, false);
    drawValue(82, "ERROR", HUMI_C, false);
  }
  snprintf(value, sizeof(value), "%d %%", lightPercent);
  drawValue(134, value, LIGHT_C, false);
  drawStatusBar();
}

bool readMqttByte(uint8_t& value, unsigned long timeout = 3000) {
  unsigned long start = millis();
  while (!mqttClient.available() && millis() - start < timeout) delay(1);
  if (!mqttClient.available()) return false;
  value = mqttClient.read();
  return true;
}

bool waitForMqttPacket(uint8_t& packetType, uint32_t& remainingLength,
                       uint8_t& header) {
  if (!readMqttByte(header)) return false;
  packetType = header >> 4;
  remainingLength = 0;
  uint32_t multiplier = 1;
  uint8_t encoded;
  do {
    if (!readMqttByte(encoded)) return false;
    remainingLength += (encoded & 127) * multiplier;
    multiplier *= 128;
  } while (encoded & 128);
  return true;
}

bool mqttConnect() {
  if (mqttClient.connected()) return true;
  if (WiFi.status() != WL_CONNECTED) return false;

  drawConnectionMessage("MQTT CONNECTING", MQTT_HOST);
  mqttClient.stop();
  if (!mqttClient.connect(MQTT_HOST, MQTT_PORT)) {
    Serial.println("MQTT TCP connection failed");
    drawCurrentPage();
    return false;
  }

  String clientId = makeClientId();
  uint16_t connectLength = 10 + 2 + clientId.length();
  mqttClient.write((uint8_t)0x10);
  writeMqttLength(connectLength);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x04);
  writeMqttString("MQTT");
  mqttClient.write((uint8_t)0x04); // MQTT 3.1.1
  mqttClient.write((uint8_t)0x02); // clean session
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x3C);
  mqttClient.write((uint8_t)((clientId.length() >> 8) & 0xFF));
  mqttClient.write((uint8_t)(clientId.length() & 0xFF));
  writeMqttString(clientId);

  uint8_t packetType;
  uint32_t remainingLength;
  uint8_t header;
  if (!waitForMqttPacket(packetType, remainingLength, header) ||
      packetType != 2 || remainingLength < 2) {
    mqttClient.stop();
    Serial.println("MQTT CONNACK failed");
    drawCurrentPage();
    return false;
  }

  uint8_t ackFlags;
  uint8_t returnCode;
  if (!readMqttByte(ackFlags) || !readMqttByte(returnCode)) {
    mqttClient.stop();
      drawCurrentPage();
    return false;
  }
  while (remainingLength > 2) {
    if (!readMqttByte(header)) {
      mqttClient.stop();
      drawCurrentPage();
      return false;
    }
    remainingLength--;
  }

  if (returnCode != 0) {
    mqttClient.stop();
    Serial.printf("MQTT CONNACK error: %u\n", returnCode);
    drawCurrentPage();
    return false;
  }

  uint16_t gledTopicLength = strlen(MQTT_GLED_TOPIC);
  uint16_t yledTopicLength = strlen(MQTT_YLED_TOPIC);
  uint16_t rledTopicLength = strlen(MQTT_RLED_TOPIC);
  uint32_t subscribeLength = 2 +
                             (2 + gledTopicLength + 1) +
                             (2 + yledTopicLength + 1) +
                             (2 + rledTopicLength + 1);

  mqttClient.write((uint8_t)0x82); // SUBSCRIBE, QoS 1
  writeMqttLength(subscribeLength);
  mqttClient.write((uint8_t)0x00);
  mqttClient.write((uint8_t)0x01); // packet identifier

  mqttClient.write((uint8_t)((gledTopicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(gledTopicLength & 0xFF));
  writeMqttString(MQTT_GLED_TOPIC);
  mqttClient.write((uint8_t)0x00); // requested QoS 0

  mqttClient.write((uint8_t)((yledTopicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(yledTopicLength & 0xFF));
  writeMqttString(MQTT_YLED_TOPIC);
  mqttClient.write((uint8_t)0x00); // requested QoS 0

  mqttClient.write((uint8_t)((rledTopicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(rledTopicLength & 0xFF));
  writeMqttString(MQTT_RLED_TOPIC);
  mqttClient.write((uint8_t)0x00); // requested QoS 0

  if (!waitForMqttPacket(packetType, remainingLength, header) ||
      packetType != 9 || remainingLength < 3) {
    mqttClient.stop();
    Serial.println("MQTT SUBACK failed");
    drawCurrentPage();
    return false;
  }
  while (remainingLength > 0) {
    if (!readMqttByte(header)) {
      mqttClient.stop();
    drawCurrentPage();
      return false;
    }
    remainingLength--;
  }

  lastPing = millis();
  Serial.println("MQTT connected");
  Serial.println("MQTT subscribed:");
  Serial.println(MQTT_GLED_TOPIC);
  Serial.println(MQTT_YLED_TOPIC);
  Serial.println(MQTT_RLED_TOPIC);
  drawCurrentPage();
  return true;
}

bool mqttPublishSensorData(byte temperature, byte humidity, int lightPercent) {
  if (!mqttClient.connected()) return false;

  char payload[64];
  snprintf(payload, sizeof(payload),
           "{\"temp\":%d,\"humi\":%d,\"light\":%d}",
           temperature, humidity, lightPercent);

  uint16_t topicLength = strlen(MQTT_DATA_TOPIC);
  uint32_t remainingLength = 2 + topicLength + strlen(payload);
  mqttClient.write((uint8_t)0x30); // PUBLISH, QoS 0
  writeMqttLength(remainingLength);
  mqttClient.write((uint8_t)((topicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(topicLength & 0xFF));
  writeMqttString(MQTT_DATA_TOPIC);
  writeMqttString(payload);

  Serial.print("Published [");
  Serial.print(MQTT_DATA_TOPIC);
  Serial.print("]: ");
  Serial.println(payload);
  return mqttClient.connected();
}

void mqttLoop() {
  if (!mqttClient.connected()) return;
  while (mqttClient.available()) {
    uint8_t packetType;
    uint32_t remainingLength;
    uint8_t header;
    if (!waitForMqttPacket(packetType, remainingLength, header)) {
      mqttClient.stop();
      return;
    }

    if (packetType == 3 && remainingLength >= 2) {
      uint8_t highByte;
      uint8_t lowByte;
      if (!readMqttByte(highByte) || !readMqttByte(lowByte)) {
        mqttClient.stop();
        return;
      }
      uint16_t topicLength = ((uint16_t)highByte << 8) | lowByte;
      if (remainingLength < 2 + topicLength) {
        mqttClient.stop();
        return;
      }

      String topic;
      for (uint16_t i = 0; i < topicLength; i++) {
        uint8_t value;
        if (!readMqttByte(value)) {
          mqttClient.stop();
          return;
        }
        topic += (char)value;
      }

      uint32_t payloadLength = remainingLength - 2 - topicLength;
      String payload;
      payload.reserve(payloadLength + 1);
      for (uint32_t i = 0; i < payloadLength; i++) {
        uint8_t value;
        if (!readMqttByte(value)) {
          mqttClient.stop();
          return;
        }
        payload += (char)value;
      }
      mqttCallback(topic.c_str(), payload);
    } else {
      while (remainingLength > 0) {
        uint8_t value;
        if (!readMqttByte(value)) {
          mqttClient.stop();
          return;
        }
        remainingLength--;
      }
    }
  }
  if (millis() - lastPing >= 20000UL) {
    mqttClient.write((uint8_t)0xC0);
    mqttClient.write((uint8_t)0x00);
    lastPing = millis();
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  drawConnectionMessage("WIFI CONNECTING", WIFI_SSID);

  for (uint8_t i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print('.');
    gfx.setTextColor(WHITE_C);
    gfx.setCursor(8 + (i % 20) * 6, 65);
    gfx.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC,
               NTP_SERVER_1, NTP_SERVER_2);
    Serial.println("NTP time synchronization started (UTC+8)");
    drawConnectionMessage("WIFI CONNECTED", WiFi.localIP().toString().c_str());
    delay(700);
  } else {
    Serial.println("WiFi connection failed");
    drawConnectionMessage("WIFI CONNECTION FAILED", "Retrying...");
    delay(700);
  }
  drawCurrentPage();
  lastWiFiAttempt = millis();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PAGE_BUTTON_PIN),
                  onPageButtonPressed, FALLING);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  updateLeds();
  SPI.begin(TFT_CLK, -1, TFT_SDI, TFT_CS);
  tft.begin(SPI);
  tft.setOrientation(0);
  drawStaticDashboard();
  Serial.println("ILI9225 MQTT dashboard started");
  connectWiFi();
  mqttConnect();
  drawStaticDashboard();
}

void loop() {
  handlePageButton();

  if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttempt >= 2000UL) {
    mqttClient.stop();
    connectWiFi();
  }

  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
    mqttConnect();
  }
  mqttLoop();

  byte temperature = 0;
  byte humidity = 0;
  bool dhtOk = dht11.read(&temperature, &humidity, nullptr) == SimpleDHTErrSuccess;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  bool humidityAlert = dhtOk && humidity > 65;
  latestTemperature = temperature;
  latestTemperatureValid = dhtOk;
  latestHumidity = humidity;
  latestHumidityValid = dhtOk;
  latestLightPercent = lightPercent;

  if (dhtOk && mqttClient.connected() &&
      (lastDataPublish == 0 || millis() - lastDataPublish >= MQTT_DATA_INTERVAL)) {
    mqttPublishSensorData(temperature, humidity, lightPercent);
    lastDataPublish = millis();
  }

  bool newHistorySample = lastTemperatureSample == 0 ||
      millis() - lastTemperatureSample >= TEMPERATURE_SAMPLE_INTERVAL;
  if (newHistorySample) {
    if (dhtOk) {
      recordTemperature(temperature);
      recordHumidity(humidity);
    }
    recordLight(lightPercent);
    lastTemperatureSample = millis();
    if (currentPage == 1) {
      drawTemperatureChart();
    } else if (currentPage == 2) {
      drawCurrentPage();
    } else if (currentPage == 3) {
      drawCurrentPage();
    }
  }

  if (currentPage == 3 &&
      (lastDetailPageDraw == 0 || millis() - lastDetailPageDraw >= 1000UL)) {
    drawCurrentPage();
    lastDetailPageDraw = millis();
  }

  if (currentPage == 0 &&
      (lastClockUpdate == 0 || millis() - lastClockUpdate >= 10000UL)) {
    drawDateTime();
    lastClockUpdate = millis();
  }

  for (uint8_t blink = 0; blink < 4; blink++) {
    handlePageButton();
    if (currentPage == 0) {
      updateValues(temperature, humidity, lightPercent, dhtOk,
                   humidityAlert && (blink % 2 == 1));
    }
    mqttLoop();
    delay(500);
  }

  Serial.printf("Temperature: %d C, humidity: %d %%, brightness: %d %%\n",
                temperature, humidity, lightPercent);
}

