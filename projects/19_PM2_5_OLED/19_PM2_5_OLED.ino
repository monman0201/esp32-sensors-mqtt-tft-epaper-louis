#include <WiFi.h>//WiFi
#include <HTTPClient.h>//瀏覽器
#include <ArduinoJson.h>//請先安裝ArduinoJson程式庫

#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

char ssid[] = "YOUR_WIFI_SSID"; //請修改為您連線的網路名稱
char password[] = "YOUR_WIFI_PASSWORD"; //請修改為您連線的網路密碼
char url[] = "https://data.moenv.gov.tw/api/v2/aqx_p_02?api_key=234630f5-da55-45f8-a1c1-7a28895857a6&limit=100"; //讀取的網址及授權密碼

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("開始連線到無線網路SSID:");

  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始
  Serial.println(ssid);
    //1.設定WiFi模式
  WiFi.mode(WIFI_STA);
  //2.啟動WiFi連線
  WiFi.begin(ssid, password);
  //3.檢查連線狀態
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    }
  Serial.println("連線完成");
}

void loop() {
  //4.啟動網頁連線
  Serial.println("啟動網頁連線");
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("httpCode=");
  Serial.println(httpCode);
  //5.檢查網頁連線是否正常
  if (httpCode == HTTP_CODE_OK) {
    //6.取得網頁內容
    String payload = http.getString();
    Serial.print("payload=");
    //7.將資料顯示在螢幕上
    Serial.println(payload);
    //JSON格式解析
    DynamicJsonDocument AQIJson(payload.length() * 2); //宣告一個JSON文件，名稱為AQIJson
    deserializeJson(AQIJson, payload);//解析網頁內容payload為JSON格式，存放在AQIJson內
    for (int i = 0; i < AQIJson.size(); i++) {
	  // 瀏覽records內的所有紀錄，直到找到site=="橋頭"
      if (AQIJson[i]["site"] == "前鎮") {        
        String AQI = AQIJson[i]["pm25"];
        Serial.println("前鎮 PM2.5=" + AQI);
		            u8g2.clearBuffer();                        //顯示前清除螢幕
        u8g2.setCursor(0, 5);                     //移動游標
        u8g2.print("智慧電子杜建平");           //寫入文字

        u8g2.setCursor(0, 25);                     //移動游標
        u8g2.print("前鎮區空氣品質" );           //寫入文字 

        u8g2.setCursor(0, 45);                     //移動游標
        u8g2.print("PM2.5=" + AQI+"ug");           //寫入文字
      
        u8g2.sendBuffer();  //送到螢幕顯示

      delay(5000);
      break;
      }
    }
   
  }
  http.end();
  delay(30000);
}

