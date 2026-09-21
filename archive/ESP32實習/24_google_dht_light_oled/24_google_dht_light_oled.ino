#include <Wire.h>
#include "D:/simpledht/SimpleDHT.h"
#include "D:/simpledht/SimpleDHT.cpp"
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

const int DHT11_PIN = 14;
const int LIGHT_PIN = 33;
const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* GOOGLE_SCRIPT_ID = "YOUR_GOOGLE_SCRIPT_ID";
const char* GOOGLE_SHEET_ID = "YOUR_GOOGLE_SHEET_ID";
const char* GOOGLE_SHEET_NAME = "data";
const unsigned long GOOGLE_SHEET_INTERVAL = 10000;

SimpleDHT11 dht11(DHT11_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

unsigned long lastGoogleSheetUpload = 0;
bool googleSheetUploadOk = false;
bool googleSheetUploading = false;

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

String urlEncode(const String& value) {
  const char* hex = "0123456789ABCDEF";
  String encoded;

  for (unsigned int i = 0; i < value.length(); i++) {
    unsigned char c = value[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      encoded += char(c);
    } else {
      encoded += '%';
      encoded += hex[c >> 4];
      encoded += hex[c & 0x0F];
    }
  }
  return encoded;
}

bool uploadToGoogleSheets(byte temperature, byte humidity, int brightness) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure sheetClient;
  sheetClient.setInsecure();
  sheetClient.setTimeout(5000);

  const char* host = "script.google.com";
  if (!sheetClient.connect(host, 443)) {
    Serial.println("Google Sheets connection failed");
    return false;
  }

  String data = String(temperature) + "," + String(humidity) + "," + String(brightness);
  String url = "/macros/s/";
  url += GOOGLE_SCRIPT_ID;
  url += "/exec?type=insert&dateInclude=1&sheetId=";
  url += urlEncode(GOOGLE_SHEET_ID);
  url += "&sheetTag=";
  url += urlEncode(GOOGLE_SHEET_NAME);
  url += "&data=";
  url += urlEncode(data);

  sheetClient.println(String("GET ") + url + " HTTP/1.1");
  sheetClient.println(String("Host: ") + host);
  sheetClient.println("Accept: */*");
  sheetClient.println("Connection: close");
  sheetClient.println();

  String statusLine = sheetClient.readStringUntil('\n');
  Serial.print("Google Sheets response: ");
  Serial.println(statusLine);
  sheetClient.stop();
  return statusLine.startsWith("HTTP/1.1 200") || statusLine.startsWith("HTTP/1.1 302");
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
  oled.setCursor(108, 43);
  if (googleSheetUploading) {
    oled.print("UP");
  } else if (googleSheetUploadOk) {
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

  drawDashboard(temperature, humidity, lightPercent, dhtOk);

  if (dhtOk && (lastGoogleSheetUpload == 0 || millis() - lastGoogleSheetUpload >= GOOGLE_SHEET_INTERVAL)) {
    googleSheetUploading = true;
    drawDashboard(temperature, humidity, lightPercent, dhtOk);
    googleSheetUploadOk = uploadToGoogleSheets(temperature, humidity, lightPercent);
    googleSheetUploading = false;
    lastGoogleSheetUpload = millis();
    drawDashboard(temperature, humidity, lightPercent, dhtOk);
  }

  Serial.print("Temperature: ");
  if (dhtOk) {
    Serial.print(temperature);
    Serial.print(", humidity: ");
    Serial.print(humidity);
    Serial.print(" %, ");
  } else {
    Serial.print("ERROR, humidity: ERROR, ");
  }
  Serial.print("brightness: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  delay(1000);
}

