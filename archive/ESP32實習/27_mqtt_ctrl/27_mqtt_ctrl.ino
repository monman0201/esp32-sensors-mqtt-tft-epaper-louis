#include <Wire.h>
#include <SimpleDHT.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <WiFi.h>

const int DHT11_PIN = 14;
const int LIGHT_PIN = 33;
const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;

const int GREEN_LED_PIN = 15;
const int YELLOW_LED_PIN = 2;
const int RED_LED_PIN = 4;

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_HOST = "mqttgo.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_GLED_TOPIC = "louis/class305/ctrl/gled";
const char* MQTT_YLED_TOPIC = "louis/class305/ctrl/yled";
const char* MQTT_RLED_TOPIC = "louis/class305/ctrl/rled";
const char* MQTT_DATA_TOPIC = "louis/class305/data";
const unsigned long MQTT_DATA_INTERVAL = 10000;

SimpleDHT11 dht11(DHT11_PIN);
WiFiClient mqttClient;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

String mqttClientId;
bool greenOn = false;
bool yellowOn = false;
bool redOn = false;
String lastCommand = "Waiting...";
unsigned long commandShownUntil = 0;
unsigned long lastMqttActivity = 0;
unsigned long lastPing = 0;
unsigned long lastDataPublish = 0;

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
  return "esp32-ctrl-" + String(esp_random(), HEX);
}

void updateLeds() {
  digitalWrite(GREEN_LED_PIN, greenOn ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellowOn ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, redOn ? HIGH : LOW);
}

void drawDashboard() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.drawStr(2, 11, "MQTT CONTROL");
  oled.drawStr(2, 27, greenOn ? "GLED: ON" : "GLED: OFF");
  oled.drawStr(2, 41, yellowOn ? "YLED: ON" : "YLED: OFF");
  oled.drawStr(2, 55, redOn ? "RLED: ON" : "RLED: OFF");
  oled.setCursor(92, 11);
  oled.print(mqttClient.connected() ? "MQ OK" : "MQ --");
  oled.sendBuffer();
}

void showCommand(const String& command) {
  lastCommand = command;
  commandShownUntil = millis() + 1000;
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.drawStr(20, 18, "MQTT COMMAND");
  oled.setFont(u8g2_font_7x13B_tf);
  oled.setCursor(18, 43);
  oled.print(command);
  oled.sendBuffer();
}

void mqttCallback(const char* topic, const String& payload) {
  StaticJsonDocument<128> document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return;
  }

  bool changed = false;
  String commandText;
  String receivedTopic = topic;
  if (receivedTopic == MQTT_GLED_TOPIC && document.containsKey("gled")) {
    greenOn = document["gled"].as<String>() == "on";
    commandText = String("GLED ") + (greenOn ? "ON" : "OFF");
    changed = true;
  }
  if (receivedTopic == MQTT_YLED_TOPIC && document.containsKey("yled")) {
    yellowOn = document["yled"].as<String>() == "on";
    commandText = String("YLED ") + (yellowOn ? "ON" : "OFF");
    changed = true;
  }
  if (receivedTopic == MQTT_RLED_TOPIC && document.containsKey("rled")) {
    redOn = document["rled"].as<String>() == "on";
    commandText = String("RLED ") + (redOn ? "ON" : "OFF");
    changed = true;
  }

  if (changed) {
    updateLeds();
    showCommand(commandText);
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(payload);
  }
}

bool readMqttByte(uint8_t& value, unsigned long timeout = 3000) {
  unsigned long start = millis();
  while (!mqttClient.available() && millis() - start < timeout) delay(1);
  if (!mqttClient.available()) return false;
  value = mqttClient.read();
  return true;
}

bool waitForMqttPacket(uint8_t& packetType, uint32_t& remainingLength, uint8_t& header) {
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

bool mqttConnectAndSubscribe() {
  if (mqttClient.connected()) return true;
  mqttClient.stop();
  if (!mqttClient.connect(MQTT_HOST, MQTT_PORT)) {
    Serial.println("MQTT TCP connection failed");
    return false;
  }

  mqttClientId = makeClientId();
  uint16_t connectLength = 10 + 2 + mqttClientId.length();
  mqttClient.write((uint8_t)0x10);
  writeMqttLength(connectLength);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x04);
  writeMqttString("MQTT");
  mqttClient.write((uint8_t)0x04);
  mqttClient.write((uint8_t)0x02);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x3C);
  mqttClient.write((uint8_t)((mqttClientId.length() >> 8) & 0xFF));
  mqttClient.write((uint8_t)(mqttClientId.length() & 0xFF));
  writeMqttString(mqttClientId);

  uint8_t packetType;
  uint32_t remainingLength;
  uint8_t header;
  if (!waitForMqttPacket(packetType, remainingLength, header) || packetType != 2 || remainingLength < 2) {
    mqttClient.stop();
    Serial.println("MQTT CONNACK failed");
    return false;
  }
  uint8_t protocolLevel;
  uint8_t returnCode;
  if (!readMqttByte(protocolLevel) || !readMqttByte(returnCode)) {
    mqttClient.stop();
    return false;
  }
  while (remainingLength > 2) { if (!readMqttByte(protocolLevel)) return false; remainingLength--; }
  if (returnCode != 0) {
    mqttClient.stop();
    Serial.print("MQTT CONNACK error: ");
    Serial.println(returnCode);
    return false;
  }

  uint16_t gledTopicLength = strlen(MQTT_GLED_TOPIC);
  uint16_t yledTopicLength = strlen(MQTT_YLED_TOPIC);
  uint16_t rledTopicLength = strlen(MQTT_RLED_TOPIC);
  uint16_t subscribeLength = 2 +
                             (2 + gledTopicLength + 1) +
                             (2 + yledTopicLength + 1) +
                             (2 + rledTopicLength + 1);
  mqttClient.write((uint8_t)0x82);
  writeMqttLength(subscribeLength);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x01);
  mqttClient.write((uint8_t)((gledTopicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(gledTopicLength & 0xFF));
  writeMqttString(MQTT_GLED_TOPIC);
  mqttClient.write((uint8_t)0x00); // QoS 0
  mqttClient.write((uint8_t)((yledTopicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(yledTopicLength & 0xFF));
  writeMqttString(MQTT_YLED_TOPIC);
  mqttClient.write((uint8_t)0x00); // QoS 0
  mqttClient.write((uint8_t)((rledTopicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(rledTopicLength & 0xFF));
  writeMqttString(MQTT_RLED_TOPIC);
  mqttClient.write((uint8_t)0x00); // QoS 0

  if (!waitForMqttPacket(packetType, remainingLength, header) || packetType != 9) {
    mqttClient.stop();
    Serial.println("MQTT SUBACK failed");
    return false;
  }
  while (remainingLength > 0) { if (!readMqttByte(header)) return false; remainingLength--; }
  lastMqttActivity = millis();
  lastPing = millis();
  Serial.println("MQTT subscribed:");
  Serial.println(MQTT_GLED_TOPIC);
  Serial.println(MQTT_YLED_TOPIC);
  Serial.println(MQTT_RLED_TOPIC);
  return true;
}

bool mqttPublishSensorData(byte temperature, byte humidity, int lightPercent) {
  if (!mqttClient.connected()) return false;

  StaticJsonDocument<96> document;
  document["temp"] = temperature;
  document["humi"] = humidity;
  document["light"] = lightPercent;
  String payload;
  serializeJson(document, payload);

  uint16_t topicLength = strlen(MQTT_DATA_TOPIC);
  uint32_t remainingLength = 2 + topicLength + payload.length();
  mqttClient.write((uint8_t)0x30); // PUBLISH, QoS 0
  writeMqttLength(remainingLength);
  mqttClient.write((uint8_t)((topicLength >> 8) & 0xFF));
  mqttClient.write((uint8_t)(topicLength & 0xFF));
  writeMqttString(MQTT_DATA_TOPIC);
  writeMqttString(payload);
  lastMqttActivity = millis();

  Serial.print("Published [");
  Serial.print(MQTT_DATA_TOPIC);
  Serial.print("]: ");
  Serial.println(payload);
  return mqttClient.connected();
}

void mqttLoop() {
  if (!mqttClient.connected()) return;
  if (mqttClient.available()) {
    uint8_t packetType;
    uint32_t remainingLength;
    uint8_t header;
    if (!waitForMqttPacket(packetType, remainingLength, header)) return;
    lastMqttActivity = millis();
    if (packetType == 3 && remainingLength >= 2) {
      uint8_t highByte;
      uint8_t lowByte;
      if (!readMqttByte(highByte) || !readMqttByte(lowByte)) { mqttClient.stop(); return; }
      uint16_t topicLength = ((uint16_t)highByte << 8) | lowByte;
      String topic;
      for (uint16_t i = 0; i < topicLength; i++) {
        uint8_t value;
        if (!readMqttByte(value)) { mqttClient.stop(); return; }
        topic += (char)value;
      }
      uint32_t payloadLength = remainingLength - 2 - topicLength;
      String payload;
      for (uint32_t i = 0; i < payloadLength; i++) {
        uint8_t value;
        if (!readMqttByte(value)) { mqttClient.stop(); return; }
        payload += (char)value;
      }
      mqttCallback(topic.c_str(), payload);
    } else {
      while (remainingLength > 0) {
        uint8_t value;
        if (!readMqttByte(value)) { mqttClient.stop(); return; }
        remainingLength--;
      }
    }
  }

  if (millis() - lastPing >= 20000) {
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
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print('.'); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  updateLeds();
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.begin();
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.drawStr(18, 28, "MQTT CTRL START");
  oled.sendBuffer();
  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) mqttConnectAndSubscribe();
  mqttLoop();

  byte temperature = 0;
  byte humidity = 0;
  bool dhtOk = dht11.read(&temperature, &humidity, nullptr) == SimpleDHTErrSuccess;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);

  if (dhtOk && mqttClient.connected() &&
      (lastDataPublish == 0 || millis() - lastDataPublish >= MQTT_DATA_INTERVAL)) {
    mqttPublishSensorData(temperature, humidity, lightPercent);
    lastDataPublish = millis();
  }

  if (commandShownUntil != 0 && millis() >= commandShownUntil) {
    commandShownUntil = 0;
    drawDashboard();
  } else if (commandShownUntil == 0) {
    drawDashboard();
  }
  delay(10);
}

