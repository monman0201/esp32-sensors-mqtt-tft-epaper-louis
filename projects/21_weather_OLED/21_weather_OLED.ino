#include <WiFi.h>          // WiFi
#include <HTTPClient.h>    // HTTP
#include <ArduinoJson.h>   // ArduinoJson

// OLED
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// WiFi設定
char ssid[] = "YOUR_WIFI_SSID";
char password[] = "YOUR_WIFI_PASSWORD";

// PM2.5 API
char url[] = "http://opendata.cwa.gov.tw/api/v1/rest/datastore/O-A0001-001?Authorization=CWA-94D8B277-86F4-4672-8061-500DCAF72AE3";

void setup() {

  Serial.begin(115200);
  delay(1000);

  // OLED初始化
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_unifont_t_chinese1);
  u8g2.setFontPosTop();

  u8g2.clearBuffer();
  u8g2.setCursor(0, 5);
  u8g2.print("小霸王有限公司");

  u8g2.setCursor(0, 25);
  u8g2.print("WiFi連線中...");

  u8g2.sendBuffer();

  Serial.print("開始連線到無線網路SSID:");
  Serial.println(ssid);

  // WiFi連線
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("連線完成");

  u8g2.clearBuffer();
  u8g2.setCursor(0, 5);
  u8g2.print("小霸王有限公司");

  u8g2.setCursor(0, 25);
  u8g2.print("WiFi連線成功");

  u8g2.sendBuffer();

  delay(1000);
}

void loop() {

  Serial.println("啟動網頁連線");

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();

  Serial.print("httpCode=");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK) {

    String payload = http.getString();

    DynamicJsonDocument AQIJson(payload.length() * 2);

    deserializeJson(AQIJson, payload);

    for (int i = 0; i < AQIJson.size(); i++) {

      if (AQIJson[i]["site"] == "林園") {

        String AQI = AQIJson[i]["pm25"];

        Serial.println("林園 PM2.5=" + AQI);

        // OLED顯示
        u8g2.clearBuffer();

        u8g2.setCursor(0, 5);
        u8g2.print("小霸王有限公司");

        u8g2.setCursor(0, 25);
        u8g2.print("林園區空氣品質");

        u8g2.setCursor(0, 45);
        u8g2.print("PM2.5=");
        u8g2.print(AQI);

        u8g2.sendBuffer();

        break;
      }
    }
  }
  else {

    Serial.println("讀取失敗");

    u8g2.clearBuffer();

    u8g2.setCursor(0, 5);
    u8g2.print("小霸王有限公司");

    u8g2.setCursor(0, 30);
    u8g2.print("API讀取失敗");

    u8g2.sendBuffer();
  }

  http.end();

  // 30秒更新一次
  delay(30000);
}

