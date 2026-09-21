#include <Arduino.h>
#include <SimpleDHT.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "epd2in9b_V4.h"

constexpr uint16_t BUFFER_BYTES = (EPD_WIDTH * EPD_HEIGHT) / 8;
constexpr uint16_t DISPLAY_WIDTH = 296;
constexpr uint16_t DISPLAY_HEIGHT = 128;
constexpr int DHT11_PIN = 14;
constexpr int LIGHT_PIN = 33;
constexpr unsigned long UPDATE_INTERVAL = 60000;
constexpr unsigned long MQTT_DATA_INTERVAL = 10000;
constexpr unsigned long SENSOR_READ_INTERVAL = 2000;
constexpr unsigned long MQTT_RECONNECT_INTERVAL = 5000;
constexpr int GREEN_LED_PIN = 15;
constexpr int YELLOW_LED_PIN = 2;
constexpr int RED_LED_PIN = 4;

// Wi-Fi / MQTT 課程設定
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_DATA_TOPIC = "louis/class305/data";
const char* MQTT_GLED_TOPIC = "louis/class305/ctrl/gled";
const char* MQTT_YLED_TOPIC = "louis/class305/ctrl/yled";
const char* MQTT_RLED_TOPIC = "louis/class305/ctrl/rled";

uint8_t blackImage[BUFFER_BYTES];
uint8_t redImage[BUFFER_BYTES];
SimpleDHT11 dht11(DHT11_PIN);
Epd epd;
WiFiClient mqttClient;
unsigned long lastMqttPublish = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMqttPing = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastSensorRead = 0;
byte latestTemperature = 0;
byte latestHumidity = 0;
int latestLightPercent = 0;
bool latestDhtOk = false;
bool mqttPingOutstanding = false;
bool greenLedOn = false;
bool yellowLedOn = false;
bool redLedOn = false;

enum PixelColor { WHITE_PIXEL, BLACK_PIXEL, RED_PIXEL };

const uint8_t gSpace[] PROGMEM = {0,0,0,0,0,0,0};
const uint8_t gA[] PROGMEM = {14,17,17,31,17,17,17};
const uint8_t gC[] PROGMEM = {15,16,16,16,16,16,15};
const uint8_t gE[] PROGMEM = {31,16,16,30,16,16,31};
const uint8_t gG[] PROGMEM = {14,17,16,23,17,17,14};
const uint8_t gH[] PROGMEM = {17,17,17,31,17,17,17};
const uint8_t gI[] PROGMEM = {31,4,4,4,4,4,31};
const uint8_t gL[] PROGMEM = {16,16,16,16,16,16,31};
const uint8_t gM[] PROGMEM = {17,27,21,21,17,17,17};
const uint8_t gN[] PROGMEM = {17,25,21,19,17,17,17};
const uint8_t gO[] PROGMEM = {14,17,17,17,17,17,14};
const uint8_t gP[] PROGMEM = {30,17,17,30,16,16,16};
const uint8_t gR[] PROGMEM = {30,17,17,30,20,18,17};
const uint8_t gT[] PROGMEM = {31,4,4,4,4,4,4};
const uint8_t gU[] PROGMEM = {17,17,17,17,17,17,14};
const uint8_t gV[] PROGMEM = {17,17,17,17,17,10,4};
const uint8_t g0[] PROGMEM = {14,17,19,21,25,17,14};
const uint8_t g1[] PROGMEM = {4,12,4,4,4,4,14};
const uint8_t g2[] PROGMEM = {14,17,1,2,4,8,31};
const uint8_t g3[] PROGMEM = {30,1,1,14,1,1,30};
const uint8_t g4[] PROGMEM = {2,6,10,18,31,2,2};
const uint8_t g5[] PROGMEM = {31,16,16,30,1,1,30};
const uint8_t g6[] PROGMEM = {14,16,16,30,17,17,14};
const uint8_t g7[] PROGMEM = {31,1,2,4,8,8,8};
const uint8_t g8[] PROGMEM = {14,17,17,14,17,17,14};
const uint8_t g9[] PROGMEM = {14,17,17,15,1,1,14};
const uint8_t gColon[] PROGMEM = {0,4,4,0,4,4,0};
const uint8_t gPercent[] PROGMEM = {17,2,4,8,17,0,0};
const uint8_t gDash[] PROGMEM = {0,0,0,31,0,0,0};

const uint8_t* glyphFor(char c) {
  switch (c) {
    case 'A': return gA; case 'C': return gC; case 'E': return gE;
    case 'G': return gG; case 'H': return gH; case 'I': return gI;
    case 'L': return gL; case 'M': return gM; case 'N': return gN;
    case 'O': return gO; case 'P': return gP; case 'R': return gR;
    case 'T': return gT; case 'U': return gU; case 'V': return gV;
    case '0': return g0; case '1': return g1; case '2': return g2;
    case '3': return g3; case '4': return g4; case '5': return g5;
    case '6': return g6; case '7': return g7; case '8': return g8;
    case '9': return g9; case ':': return gColon; case '%': return gPercent;
    case '-': return gDash; default: return gSpace;
  }
}

void setPixel(uint16_t x, uint16_t y, PixelColor color) {
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
  const uint16_t driverX = y;
  const uint16_t driverY = DISPLAY_WIDTH - 1 - x;
  const uint16_t index = driverY * (EPD_WIDTH / 8) + driverX / 8;
  const uint8_t mask = 0x80 >> (driverX % 8);

  if (color == BLACK_PIXEL) {
    blackImage[index] &= static_cast<uint8_t>(~mask);
    redImage[index] |= mask;
  } else if (color == RED_PIXEL) {
    blackImage[index] |= mask;
    redImage[index] &= static_cast<uint8_t>(~mask);
  } else {
    blackImage[index] |= mask;
    redImage[index] |= mask;
  }
}

void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, PixelColor color) {
  for (uint16_t yy = y; yy < y + h; ++yy)
    for (uint16_t xx = x; xx < x + w; ++xx)
      setPixel(xx, yy, color);
}

void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, PixelColor color) {
  for (uint16_t xx = x; xx < x + w; ++xx) {
    setPixel(xx, y, color);
    setPixel(xx, y + h - 1, color);
  }
  for (uint16_t yy = y; yy < y + h; ++yy) {
    setPixel(x, yy, color);
    setPixel(x + w - 1, yy, color);
  }
}

void fillCircle(int16_t cx, int16_t cy, int16_t radius, PixelColor color) {
  for (int16_t y = -radius; y <= radius; ++y) {
    for (int16_t x = -radius; x <= radius; ++x) {
      if (x * x + y * y <= radius * radius) {
        setPixel(cx + x, cy + y, color);
      }
    }
  }
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, PixelColor color) {
  const int16_t dx = abs(x1 - x0);
  const int16_t sx = x0 < x1 ? 1 : -1;
  const int16_t dy = -abs(y1 - y0);
  const int16_t sy = y0 < y1 ? 1 : -1;
  int16_t error = dx + dy;
  while (true) {
    setPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    const int16_t e2 = 2 * error;
    if (e2 >= dy) { error += dy; x0 += sx; }
    if (e2 <= dx) { error += dx; y0 += sy; }
  }
}

void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                   PixelColor color, uint8_t thickness) {
  for (int8_t offset = -static_cast<int8_t>(thickness / 2);
       offset <= static_cast<int8_t>(thickness / 2); ++offset) {
    if (abs(x1 - x0) >= abs(y1 - y0))
      drawLine(x0, y0 + offset, x1, y1 + offset, color);
    else
      drawLine(x0 + offset, y0, x1 + offset, y1, color);
  }
}

void drawThermometer(uint16_t cx, uint16_t y) {
  // Preview-style thermometer: black outline, white tube, red liquid and scale marks.
  fillRect(cx - 6, y, 12, 16, BLACK_PIXEL);
  fillRect(cx - 3, y + 2, 6, 13, WHITE_PIXEL);
  fillCircle(cx, y + 17, 9, BLACK_PIXEL);
  fillCircle(cx, y + 17, 6, WHITE_PIXEL);
  fillRect(cx - 2, y + 5, 4, 12, RED_PIXEL);
  fillCircle(cx, y + 17, 5, RED_PIXEL);
  fillRect(cx + 12, y + 3, 9, 3, BLACK_PIXEL);
  fillRect(cx + 12, y + 9, 7, 3, BLACK_PIXEL);
  fillRect(cx + 12, y + 15, 9, 3, BLACK_PIXEL);
}

void drawDroplet(uint16_t cx, uint16_t y) {
  // Pointed black outer drop with a rounded lower half.
  const int8_t outerHalf[] = {
    1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8,
    8, 8, 8, 8, 7, 6, 5, 4, 3, 2, 1, 0
  };
  for (uint8_t row = 0; row < sizeof(outerHalf); ++row) {
    fillRect(cx - outerHalf[row], y + row, outerHalf[row] * 2 + 1, 1, BLACK_PIXEL);
  }
  const int8_t innerHalf[] = {1, 2, 3, 4, 5, 5, 5, 5, 4, 3, 2, 1, 0};
  for (uint8_t row = 0; row < sizeof(innerHalf); ++row) {
    fillRect(cx - innerHalf[row], y + 7 + row,
             innerHalf[row] * 2 + 1, 1, RED_PIXEL);
  }
}

void drawSun(uint16_t cx, uint16_t cy) {
  // Preview-style sun: black ring, red center and eight bold rays.
  fillCircle(cx, cy, 10, BLACK_PIXEL);
  fillCircle(cx, cy, 7, RED_PIXEL);
  drawThickLine(cx, cy - 12, cx, cy - 16, BLACK_PIXEL, 3);
  drawThickLine(cx, cy + 12, cx, cy + 16, BLACK_PIXEL, 3);
  drawThickLine(cx - 12, cy, cx - 16, cy, BLACK_PIXEL, 3);
  drawThickLine(cx + 12, cy, cx + 16, cy, BLACK_PIXEL, 3);
  drawThickLine(cx - 9, cy - 9, cx - 13, cy - 13, BLACK_PIXEL, 3);
  drawThickLine(cx + 9, cy - 9, cx + 13, cy - 13, BLACK_PIXEL, 3);
  drawThickLine(cx - 9, cy + 9, cx - 13, cy + 13, BLACK_PIXEL, 3);
  drawThickLine(cx + 9, cy + 9, cx + 13, cy + 13, BLACK_PIXEL, 3);
}

void drawChar(uint16_t x, uint16_t y, char c, uint8_t scale, PixelColor color) {
  const uint8_t* glyph = glyphFor(c);
  for (uint8_t row = 0; row < 7; ++row) {
    const uint8_t bits = pgm_read_byte(&glyph[row]);
    for (uint8_t col = 0; col < 5; ++col) {
      if (bits & (1 << (4 - col))) {
        fillRect(x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}

uint16_t textWidth(const char* text, uint8_t scale) {
  return strlen(text) * 6 * scale - scale;
}

void drawTextCentered(const char* text, uint16_t y, uint8_t scale, PixelColor color) {
  const uint16_t width = textWidth(text, scale);
  uint16_t x = (DISPLAY_WIDTH - width) / 2;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    drawChar(x, y, text[i], scale, color);
    x += 6 * scale;
  }
}

void drawTextInBox(const char* text, uint16_t boxX, uint16_t boxW,
                   uint16_t y, uint8_t scale, PixelColor color) {
  const uint16_t width = textWidth(text, scale);
  uint16_t x = boxX + (boxW - width) / 2;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    drawChar(x, y, text[i], scale, color);
    x += 6 * scale;
  }
}


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
  if (deserializeJson(document, payload)) {
    Serial.println("MQTT JSON format error");
    return;
  }

  String receivedTopic = topic;
  bool changed = false;
  if (receivedTopic == MQTT_GLED_TOPIC && !document["gled"].isNull()) {
    String value = document["gled"].as<String>();
    if (value == "on" || value == "off") {
      greenLedOn = value == "on";
      changed = true;
    }
  } else if (receivedTopic == MQTT_YLED_TOPIC && !document["yled"].isNull()) {
    String value = document["yled"].as<String>();
    if (value == "on" || value == "off") {
      yellowLedOn = value == "on";
      changed = true;
    }
  } else if (receivedTopic == MQTT_RLED_TOPIC && !document["rled"].isNull()) {
    String value = document["rled"].as<String>();
    if (value == "on" || value == "off") {
      redLedOn = value == "on";
      changed = true;
    }
  }

  if (changed) {
    updateLeds();
    Serial.printf("LED control [%s]: %s\n", topic, payload.c_str());
  }
}

String makeClientId() {
  return "esp32-epaper-" + String((uint32_t)esp_random(), HEX);
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
  if (millis() - lastMqttAttempt < MQTT_RECONNECT_INTERVAL) return false;

  lastMqttAttempt = millis();
  Serial.println("MQTT reconnect attempt");

  mqttClient.stop();
  if (!mqttClient.connect(MQTT_HOST, MQTT_PORT)) {
    Serial.println("MQTT TCP connection failed");
    return false;
  }

  String clientId = makeClientId();
  uint16_t connectLength = 10 + 2 + clientId.length();
  mqttClient.write((uint8_t)0x10);
  writeMqttLength(connectLength);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x04);
  writeMqttString("MQTT");
  mqttClient.write((uint8_t)0x04);
  mqttClient.write((uint8_t)0x02);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x3C);
  mqttClient.write((uint8_t)((clientId.length() >> 8) & 0xFF));
  mqttClient.write((uint8_t)(clientId.length() & 0xFF));
  writeMqttString(clientId);

  uint8_t packetType, header;
  uint32_t remainingLength;
  if (!waitForMqttPacket(packetType, remainingLength, header) ||
      packetType != 2 || remainingLength < 2) {
    mqttClient.stop();
    Serial.println("MQTT CONNACK failed");
    return false;
  }

  uint8_t ackFlags, returnCode;
  if (!readMqttByte(ackFlags) || !readMqttByte(returnCode)) {
    mqttClient.stop();
    return false;
  }
  while (remainingLength > 2) {
    if (!readMqttByte(header)) {
      mqttClient.stop();
      return false;
    }
    remainingLength--;
  }
  if (returnCode != 0) {
    mqttClient.stop();
    Serial.printf("MQTT CONNACK error: %u\n", returnCode);
    return false;
  }

  const char* topics[] = {MQTT_GLED_TOPIC, MQTT_YLED_TOPIC, MQTT_RLED_TOPIC};
  uint32_t subscribeLength = 2;
  for (uint8_t i = 0; i < 3; ++i)
    subscribeLength += 2 + strlen(topics[i]) + 1;

  mqttClient.write((uint8_t)0x82);
  writeMqttLength(subscribeLength);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x01);
  for (uint8_t i = 0; i < 3; ++i) {
    uint16_t topicLength = strlen(topics[i]);
    mqttClient.write((uint8_t)((topicLength >> 8) & 0xFF));
    mqttClient.write((uint8_t)(topicLength & 0xFF));
    writeMqttString(topics[i]);
    mqttClient.write((uint8_t)0x00);
  }

  if (!waitForMqttPacket(packetType, remainingLength, header) ||
      packetType != 9 || remainingLength < 3) {
    mqttClient.stop();
    Serial.println("MQTT SUBACK failed");
    return false;
  }
  while (remainingLength > 0) {
    if (!readMqttByte(header)) {
      mqttClient.stop();
      return false;
    }
    remainingLength--;
  }

  lastMqttPing = millis();
  mqttPingOutstanding = false;
  Serial.println("MQTT connected and LED topics subscribed");
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
  mqttClient.write((uint8_t)0x30);
  writeMqttLength(remainingLength);
  mqttClient.write((uint8_t)((topicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(topicLength & 0xFF));
  writeMqttString(MQTT_DATA_TOPIC);
  writeMqttString(payload);

  Serial.printf("Published [%s]: %s\n", MQTT_DATA_TOPIC, payload);
  if (!mqttClient.connected()) {
    Serial.println("MQTT publish failed: connection lost");
    return false;
  }
  return true;
}

void mqttLoop() {
  if (!mqttClient.connected()) return;
  while (mqttClient.available()) {
    uint8_t packetType, header;
    uint32_t remainingLength;
    if (!waitForMqttPacket(packetType, remainingLength, header)) {
      mqttClient.stop();
      return;
    }

    if (packetType == 3 && remainingLength >= 2) {
      uint8_t highByte, lowByte;
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
      for (uint16_t i = 0; i < topicLength; ++i) {
        uint8_t value;
        if (!readMqttByte(value)) {
          mqttClient.stop();
          return;
        }
        topic += (char)value;
      }

      String payload;
      uint32_t payloadLength = remainingLength - 2 - topicLength;
      payload.reserve(payloadLength + 1);
      for (uint32_t i = 0; i < payloadLength; ++i) {
        uint8_t value;
        if (!readMqttByte(value)) {
          mqttClient.stop();
          return;
        }
        payload += (char)value;
      }
      mqttCallback(topic.c_str(), payload);
    } else {
      if (packetType == 13) {
        mqttPingOutstanding = false;
        Serial.println("MQTT PINGRESP received");
      }
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

  if (mqttPingOutstanding && millis() - lastMqttPing >= 10000UL) {
    Serial.println("MQTT PINGRESP timeout; reconnecting");
    mqttClient.stop();
    mqttPingOutstanding = false;
    return;
  }
  if (!mqttPingOutstanding && millis() - lastMqttPing >= 20000UL) {
    if (mqttClient.write((uint8_t)0xC0) != 1 ||
        mqttClient.write((uint8_t)0x00) != 1) {
      Serial.println("MQTT PINGREQ failed; reconnecting");
      mqttClient.stop();
      return;
    }
    mqttPingOutstanding = true;
    lastMqttPing = millis();
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting Wi-Fi: %s\n", WIFI_SSID);
  for (uint8_t i = 0; i < 40 && WiFi.status() != WL_CONNECTED; ++i)
    delay(500);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed");
  }
  lastWiFiAttempt = millis();
}

void renderDashboard(int temperature, int humidity, int lightPercent, bool dhtOk) {
  memset(blackImage, 0xFF, sizeof(blackImage));
  memset(redImage, 0xFF, sizeof(redImage));

  fillRect(0, 0, DISPLAY_WIDTH, 25, RED_PIXEL);
  drawTextCentered("ENV MONITOR", 5, 2, WHITE_PIXEL);

  const uint16_t cardY = 32;
  const uint16_t cardW = 88;
  const uint16_t cardH = 89;
  const uint16_t cardX[3] = {8, 104, 200};
  const char* labels[3] = {"TEMP", "HUMI", "LIGHT"};

  const uint16_t labelY = 73;
  for (uint8_t i = 0; i < 3; ++i) {
    drawRect(cardX[i], cardY, cardW, cardH, BLACK_PIXEL);
    fillRect(cardX[i] + 1, cardY + 1, cardW - 2, 4, RED_PIXEL);
    fillRect(cardX[i] + 6, labelY, cardW - 12, 14, RED_PIXEL);
  }

  drawThermometer(cardX[0] + cardW / 2, 42);
  drawDroplet(cardX[1] + cardW / 2, 42);
  drawSun(cardX[2] + cardW / 2, 52);
  for (uint8_t i = 0; i < 3; ++i) {
    drawTextInBox(labels[i], cardX[i] + 6, cardW - 12, labelY + 3, 1, WHITE_PIXEL);
  }

  char value[12];
  if (dhtOk) {
    snprintf(value, sizeof(value), "%d C", temperature);
    drawTextInBox(value, cardX[0], cardW, 94, 2, BLACK_PIXEL);
    snprintf(value, sizeof(value), "%d %%", humidity);
    drawTextInBox(value, cardX[1], cardW, 94, 2, BLACK_PIXEL);
  } else {
    drawTextInBox("-- C", cardX[0], cardW, 94, 2, BLACK_PIXEL);
    drawTextInBox("-- %", cardX[1], cardW, 94, 2, BLACK_PIXEL);
  }
  snprintf(value, sizeof(value), "%d %%", lightPercent);
  drawTextInBox(value, cardX[2], cardW, 94, 2, BLACK_PIXEL);
}

void updateDisplay() {
  byte temperature = 0;
  byte humidity = 0;
  const bool dhtOk = dht11.read(&temperature, &humidity, nullptr) == SimpleDHTErrSuccess;
  const int lightRaw = analogRead(LIGHT_PIN);
  const int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);

  latestTemperature = temperature;
  latestHumidity = humidity;
  latestLightPercent = lightPercent;
  latestDhtOk = dhtOk;
  lastSensorRead = millis();

  Serial.printf("Temperature: %s%d C, humidity: %s%d %%, brightness: %d %%\n",
                dhtOk ? "" : "ERROR/", temperature,
                dhtOk ? "" : "ERROR/", humidity, lightPercent);

  renderDashboard(temperature, humidity, lightPercent, dhtOk);
  if (epd.Init() == 0) {
    epd.Display(blackImage, redImage);
    epd.Sleep();
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  updateLeds();
  delay(300);

  updateDisplay();
  lastDisplayUpdate = millis();
  connectWiFi();
  mqttConnect();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastWiFiAttempt >= 2000UL) {
    mqttClient.stop();
    mqttPingOutstanding = false;
    lastMqttPublish = 0;
    connectWiFi();
  }
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected() &&
      millis() - lastMqttAttempt >= MQTT_RECONNECT_INTERVAL) {
    mqttConnect();
  }
  mqttLoop();

  if (lastSensorRead == 0 ||
      millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
    byte temperature = 0;
    byte humidity = 0;
    bool dhtOk = dht11.read(&temperature, &humidity, nullptr) == SimpleDHTErrSuccess;
    int lightRaw = analogRead(LIGHT_PIN);
    int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);

    latestTemperature = temperature;
    latestHumidity = humidity;
    latestLightPercent = lightPercent;
    latestDhtOk = dhtOk;
    lastSensorRead = millis();

    Serial.printf("Temperature: %s%d C, humidity: %s%d %%, brightness: %d %%\n",
                  dhtOk ? "" : "ERROR/", temperature,
                  dhtOk ? "" : "ERROR/", humidity, lightPercent);

    if (dhtOk && mqttClient.connected() &&
        (lastMqttPublish == 0 ||
         millis() - lastMqttPublish >= MQTT_DATA_INTERVAL)) {
      if (mqttPublishSensorData(temperature, humidity, lightPercent)) {
        lastMqttPublish = millis();
      } else {
        Serial.println("Sensor data was not published");
      }
    }
  }

  if (millis() - lastDisplayUpdate >= UPDATE_INTERVAL) {
    lastDisplayUpdate = millis();
    renderDashboard(latestTemperature, latestHumidity,
                    latestLightPercent, latestDhtOk);
    if (epd.Init() == 0) {
      epd.Display(blackImage, redImage);
      epd.Sleep();
    }
  }

  delay(50);
}

