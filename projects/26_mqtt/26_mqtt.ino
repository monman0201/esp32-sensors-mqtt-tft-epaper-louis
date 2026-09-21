#include <Wire.h>
#include <SimpleDHT.h>
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
const char* MQTT_TOPIC = "louis/class305/data";
const unsigned long MQTT_INTERVAL = 10000;

SimpleDHT11 dht11(DHT11_PIN);
WiFiClient mqttClient;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastPublish = 0;
String mqttClientId;

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
  uint32_t randomValue = esp_random();
  return "esp32-louis-" + String(randomValue, HEX);
}

bool mqttConnect() {
  if (mqttClient.connected()) return true;

  mqttClient.stop();
  if (!mqttClient.connect(MQTT_HOST, MQTT_PORT)) {
    Serial.println("MQTT TCP connection failed");
    return false;
  }

  mqttClientId = makeClientId();
  uint16_t remainingLength = 10 + 2 + mqttClientId.length();
  mqttClient.write((uint8_t)0x10);
  writeMqttLength(remainingLength);
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x04);
  writeMqttString("MQTT");
  mqttClient.write((uint8_t)0x04);       // MQTT 3.1.1
  mqttClient.write((uint8_t)0x02);       // clean session
  mqttClient.write((uint8_t)0x00); mqttClient.write((uint8_t)0x3C); // keep alive: 60 s
  mqttClient.write((mqttClientId.length() >> 8) & 0xFF);
  mqttClient.write(mqttClientId.length() & 0xFF);
  writeMqttString(mqttClientId);

  unsigned long start = millis();
  while (!mqttClient.available() && millis() - start < 3000) delay(10);
  if (!mqttClient.available()) {
    mqttClient.stop();
    Serial.println("MQTT CONNACK timeout");
    return false;
  }

  uint8_t header = mqttClient.read();
  uint8_t length = mqttClient.read();
  if (header != 0x20 || length < 2) {
    mqttClient.stop();
    Serial.println("MQTT invalid CONNACK");
    return false;
  }
  mqttClient.read();
  uint8_t returnCode = mqttClient.read();
  if (returnCode != 0) {
    mqttClient.stop();
    Serial.print("MQTT CONNACK error: ");
    Serial.println(returnCode);
    return false;
  }

  Serial.print("MQTT connected, clientId: ");
  Serial.println(mqttClientId);
  return true;
}

bool mqttPublish(const String& payload) {
  if (!mqttConnect()) return false;

  uint16_t topicLength = strlen(MQTT_TOPIC);
  uint32_t remainingLength = 2 + topicLength + payload.length();
  mqttClient.write((uint8_t)0x30); // PUBLISH, QoS 0
  writeMqttLength(remainingLength);
  mqttClient.write((topicLength >> 8) & 0xFF);
  mqttClient.write(topicLength & 0xFF);
  writeMqttString(MQTT_TOPIC);
  writeMqttString(payload);
  return mqttClient.connected();
}

void setLeds(bool green, bool yellow, bool red) {
  digitalWrite(GREEN_LED_PIN, green ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, yellow ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, red ? HIGH : LOW);
}

void updateLeds(int lightPercent, bool dhtOk, bool mqttOk) {
  if (!dhtOk || !mqttOk) {
    setLeds(false, false, true);
  } else if (lightPercent >= 60) {
    setLeds(true, false, false);
  } else if (lightPercent >= 30) {
    setLeds(false, true, false);
  } else {
    setLeds(false, false, true);
  }
}

void drawDashboard(byte temperature, byte humidity, int lightPercent, bool dhtOk, bool mqttOk) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.drawRFrame(1, 1, 61, 28, 4);
  oled.drawRFrame(66, 1, 61, 28, 4);
  oled.drawRFrame(1, 33, 126, 29, 4);

  oled.setCursor(7, 12); oled.print("TEMP");
  oled.setCursor(72, 12); oled.print("HUMI");
  oled.setCursor(8, 51); oled.print("LIGHT");

  oled.setFont(u8g2_font_7x13B_tf);
  oled.setCursor(38, 25);
  if (dhtOk) { oled.print(temperature); oled.print("C"); } else oled.print("--");
  oled.setCursor(100, 25);
  if (dhtOk) { oled.print(humidity); oled.print("%"); } else oled.print("--");
  oled.setCursor(58, 53); oled.print(lightPercent); oled.print("%");

  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(105, 60); oled.print(mqttOk ? "MQ" : "--");
  oled.sendBuffer();
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed");
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  setLeds(false, false, true);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.begin();
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.drawStr(18, 28, "MQTT START");
  oled.sendBuffer();
  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  byte temperature = 0;
  byte humidity = 0;
  bool dhtOk = dht11.read(&temperature, &humidity, nullptr) == SimpleDHTErrSuccess;
  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = constrain(map(lightRaw, 0, 4095, 0, 100), 0, 100);
  bool mqttOk = mqttClient.connected();

  if (dhtOk && (lastPublish == 0 || millis() - lastPublish >= MQTT_INTERVAL)) {
    String payload = String("{\"temp\":") + temperature +
                     ",\"humi\":" + humidity +
                     ",\"light\":" + lightPercent + "}";
    mqttOk = mqttPublish(payload);
    lastPublish = millis();
    Serial.print("MQTT publish: ");
    Serial.println(mqttOk ? payload : "failed");
  }

  updateLeds(lightPercent, dhtOk, mqttOk);
  drawDashboard(temperature, humidity, lightPercent, dhtOk, mqttOk);
  Serial.printf("Temperature: %s, humidity: %s, brightness: %d %%\n",
                dhtOk ? String(temperature).c_str() : "ERROR",
                dhtOk ? String(humidity).c_str() : "ERROR",
                lightPercent);
  delay(1000);
}

