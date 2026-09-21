#include <SPI.h>
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

// WiFi / MQTT
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_GLED_TOPIC = "louis/class305/ctrl/gled";
const char* MQTT_YLED_TOPIC = "louis/class305/ctrl/yled";
const char* MQTT_RLED_TOPIC = "louis/class305/ctrl/rled";
const char* MQTT_DATA_TOPIC = "louis/class305/data";
const unsigned long MQTT_DATA_INTERVAL = 10000UL;

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
const uint16_t GREEN_C  = COLOR_GREEN;
const uint16_t RED_C    = COLOR_RED;

unsigned long lastDataPublish = 0;
unsigned long lastPing = 0;
unsigned long lastWiFiAttempt = 0;
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

String makeClientId() {
  return "esp32-ili-" + String((uint32_t)esp_random(), HEX);
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
  String commandText;
  String receivedTopic = topic;

  if (receivedTopic == MQTT_GLED_TOPIC && !document["gled"].isNull()) {
    String value = document["gled"].as<String>();
    if (value == "on" || value == "off") {
      greenLedOn = value == "on";
      commandText = String("GLED ") + (greenLedOn ? "ON" : "OFF");
      changed = true;
    }
  } else if (receivedTopic == MQTT_YLED_TOPIC && !document["yled"].isNull()) {
    String value = document["yled"].as<String>();
    if (value == "on" || value == "off") {
      yellowLedOn = value == "on";
      commandText = String("YLED ") + (yellowLedOn ? "ON" : "OFF");
      changed = true;
    }
  } else if (receivedTopic == MQTT_RLED_TOPIC && !document["rled"].isNull()) {
    String value = document["rled"].as<String>();
    if (value == "on" || value == "off") {
      redLedOn = value == "on";
      commandText = String("RLED ") + (redLedOn ? "ON" : "OFF");
      changed = true;
    }
  }

  if (changed) {
    updateLeds();
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(payload);
    Serial.print("LED command: ");
    Serial.println(commandText);
  }
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
  gfx.fillRect(40, y + 25, 126, 27, alertFlash ? RED_C : PANEL);
  gfx.setTextSize(2);
  gfx.setTextColor(alertFlash ? WHITE_C : color);
  gfx.setCursor(43, y + 29);
  gfx.print(value);
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
  drawCardFrame(92, "HUMIDITY", HUMI_C, 1);
  drawCardFrame(154, "BRIGHTNESS", LIGHT_C, 2);
}

void updateValues(byte temperature, byte humidity, int lightPercent,
                  bool dhtOk, bool alertFlash) {
  char value[16];
  if (dhtOk) {
    snprintf(value, sizeof(value), "%d C", temperature);
    drawValue(30, value, TEMP_C, false);
    snprintf(value, sizeof(value), "%d %%", humidity);
    drawValue(92, value, HUMI_C, humidity > 65 && alertFlash);
  } else {
    drawValue(30, "ERROR", TEMP_C, false);
    drawValue(92, "ERROR", HUMI_C, false);
  }
  snprintf(value, sizeof(value), "%d %%", lightPercent);
  drawValue(154, value, LIGHT_C, false);
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
    drawStaticDashboard();
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
    drawStaticDashboard();
    return false;
  }

  uint8_t ackFlags;
  uint8_t returnCode;
  if (!readMqttByte(ackFlags) || !readMqttByte(returnCode)) {
    mqttClient.stop();
    drawStaticDashboard();
    return false;
  }
  while (remainingLength > 2) {
    if (!readMqttByte(header)) {
      mqttClient.stop();
      drawStaticDashboard();
      return false;
    }
    remainingLength--;
  }

  if (returnCode != 0) {
    mqttClient.stop();
    Serial.printf("MQTT CONNACK error: %u\n", returnCode);
    drawStaticDashboard();
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
    drawStaticDashboard();
    return false;
  }
  while (remainingLength > 0) {
    if (!readMqttByte(header)) {
      mqttClient.stop();
      drawStaticDashboard();
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
  drawStaticDashboard();
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
      topic.reserve(topicLength + 1);
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
    drawConnectionMessage("WIFI CONNECTED", WiFi.localIP().toString().c_str());
    delay(700);
  } else {
    Serial.println("WiFi connection failed");
    drawConnectionMessage("WIFI CONNECTION FAILED", "Retrying...");
    delay(700);
  }
  drawStaticDashboard();
  lastWiFiAttempt = millis();
}

void setup() {
  Serial.begin(115200);
  delay(300);
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

  if (dhtOk && mqttClient.connected() &&
      (lastDataPublish == 0 || millis() - lastDataPublish >= MQTT_DATA_INTERVAL)) {
    mqttPublishSensorData(temperature, humidity, lightPercent);
    lastDataPublish = millis();
  }

  for (uint8_t blink = 0; blink < 4; blink++) {
    updateValues(temperature, humidity, lightPercent, dhtOk,
                 humidityAlert && (blink % 2 == 1));
    mqttLoop();
    delay(500);
  }

  Serial.printf("Temperature: %d C, humidity: %d %%, brightness: %d %%\n",
                temperature, humidity, lightPercent);
}

