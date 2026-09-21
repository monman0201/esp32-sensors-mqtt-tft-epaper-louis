#include <Wire.h>
#include <SimpleDHT.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const int DHT11_PIN = 14;
const int LIGHT_PIN = 33;
const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* LINE_CHANNEL_ACCESS_TOKEN = "YOUR_LINE_CHANNEL_ACCESS_TOKEN";
const char* LINE_USER_ID = "YOUR_LINE_USER_ID";

const byte TEMPERATURE_LIMIT = 28;
const byte HUMIDITY_LIMIT = 70;
const unsigned long LINE_ALERT_INTERVAL = 30000;

SimpleDHT11 dht11(DHT11_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastLineAlert = 0;
bool lineSending = false;
bool lineAlertOk = false;

void drawTemperatureLabel(int x, int y) {
  oled.setFont(u8g2_font_wqy12_t_gb2312b);
  oled.setCursor(x, y);
  oled.print("温度");
}

void drawHumidityLabel(int x, int y) {
  oled.setFont(u8g2_font_wqy12_t_gb2312b);
  oled.setCursor(x, y);
  oled.print("湿度");
}

void drawBrightnessLabel(int x, int y) {
  oled.setFont(u8g2_font_wqy12_t_gb2312b);
  oled.setCursor(x, y);
  oled.print("亮度");
}

void showNetworkStatus(const char* message, const char* detail) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tf);
  oled.setCursor(12, 25);
  oled.print(message);
  oled.setCursor(12, 43);
  oled.print(detail);
  oled.sendBuffer();
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting to ");
  Serial.print(WIFI_SSID);

  for (int attempt = 0; attempt < 40 && WiFi.status() != WL_CONNECTED; attempt++) {
    delay(500);
    Serial.print(".");
    String progress = "Connecting";
    for (int dot = 0; dot < (attempt % 4); dot++) {
      progress += ".";
    }
    showNetworkStatus("WiFi CONNECT", progress.c_str());
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    showNetworkStatus("WiFi CONNECTED", WiFi.localIP().toString().c_str());
    delay(1200);
  } else {
    Serial.println("WiFi connection failed");
    showNetworkStatus("WiFi FAILED", "Retry later");
    delay(1200);
  }
}

bool sendLineMessage(byte temperature, byte humidity) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure lineClient;
  lineClient.setInsecure();
  lineClient.setTimeout(8000);

  String message = "環境異常警報\\n溫度: " + String(temperature) +
                   " C\\n濕度: " + String(humidity) + " %";
  String body = "{\"to\":\"" + String(LINE_USER_ID) +
                "\",\"messages\":[{\"type\":\"text\",\"text\":\"" +
                message + "\"}]}";

  if (!lineClient.connect("api.line.me", 443)) {
    Serial.println("LINE connection failed");
    return false;
  }

  lineClient.println("POST /v2/bot/message/push HTTP/1.1");
  lineClient.println("Host: api.line.me");
  lineClient.println("Connection: close");
  lineClient.println("Authorization: Bearer " + String(LINE_CHANNEL_ACCESS_TOKEN));
  lineClient.println("Content-Type: application/json; charset=utf-8");
  lineClient.println("Content-Length: " + String(body.length()));
  lineClient.println();
  lineClient.println(body);

  String statusLine = lineClient.readStringUntil('\n');
  Serial.print("LINE response: ");
  Serial.println(statusLine);
  lineClient.stop();
  return statusLine.startsWith("HTTP/1.1 200");
}

void drawDashboard(byte temperature, byte humidity, int lightPercent, bool dhtOk) {
  oled.clearBuffer();

  oled.drawRFrame(1, 1, 61, 28, 4);
  oled.drawRFrame(66, 1, 61, 28, 4);

  oled.setFont(u8g2_font_7x13B_tf);
  oled.setCursor(42, 7);
  if (dhtOk) {
    oled.print(temperature);
    oled.print("C");
  } else {
    oled.print("--");
  }

  oled.setCursor(105, 7);
  if (dhtOk) {
    oled.print(humidity);
    oled.print("%");
  } else {
    oled.print("--");
  }

  drawTemperatureLabel(7, 9);
  drawHumidityLabel(72, 9);

  oled.drawRFrame(1, 33, 126, 29, 4);
  drawBrightnessLabel(18, 41);

  oled.setFont(u8g2_font_7x13B_tf);
  oled.setCursor(64, 40);
  oled.print(lightPercent);
  oled.print("%");

  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(100, 43);
  if (lineSending) {
    oled.print("TX");
  } else if (lineAlertOk) {
    oled.print("OK");
  } else {
    oled.print("--");
  }

  oled.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.begin();
  oled.enableUTF8Print();
  oled.setFont(u8g2_font_wqy12_t_chinese1);
  oled.setFontPosTop();

  showNetworkStatus("WiFi START", "Please wait...");
  connectWiFi();
}

void loop() {
  byte temperature = 0;
  byte humidity = 0;
  int dhtError = dht11.read(&temperature, &humidity, nullptr);
  bool dhtOk = dhtError == SimpleDHTErrSuccess;

  int lightRaw = analogRead(LIGHT_PIN);
  int lightPercent = map(lightRaw, 0, 4095, 0, 100);
  lightPercent = constrain(lightPercent, 0, 100);

  bool abnormal = dhtOk && (temperature > TEMPERATURE_LIMIT || humidity > HUMIDITY_LIMIT);
  if (!abnormal) {
    lastLineAlert = 0;
    lineAlertOk = false;
  }

  drawDashboard(temperature, humidity, lightPercent, dhtOk);

  if (abnormal && (lastLineAlert == 0 || millis() - lastLineAlert >= LINE_ALERT_INTERVAL)) {
    Serial.println("異常發生，LINE正在傳訊息通知");
    lineSending = true;
    drawDashboard(temperature, humidity, lightPercent, dhtOk);
    lineAlertOk = sendLineMessage(temperature, humidity);
    lineSending = false;
    lastLineAlert = millis();
    drawDashboard(temperature, humidity, lightPercent, dhtOk);
  }

  Serial.print("Temperature: ");
  if (dhtOk) {
    Serial.print(temperature);
    Serial.print(" C, humidity: ");
    Serial.print(humidity);
    Serial.print(" %, brightness: ");
  } else {
    Serial.print("ERROR, humidity: ERROR, brightness: ");
  }
  Serial.print(lightPercent);
  Serial.println(" %");

  delay(1000);
}

