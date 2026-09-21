#include <SimpleDHT.h>
int pinDHT11 = 19;
SimpleDHT11 dht11(pinDHT11);

#include "Wire.h"
#include "U8g2lib.h"  //OLED 螢幕解析度為128*64
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

int R=25;
int G=26;
int B=27;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  u8g2.begin();                                //初始化
  u8g2.enableUTF8Print();                      //啟用 UTF8字集
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  //設定使用中文字形
  u8g2.setFontPosTop();//座標從上開始

  pinMode(R,OUTPUT);  
  pinMode(G,OUTPUT);
  pinMode(B,OUTPUT);
}

void loop() {
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
  Serial.println("Sample OK:" + (String)temperature + " *C, " +((String)humidity) + " H");


  if (humidity <= 60) {analogWrite(R,0);analogWrite(G,255);analogWrite(B,0);}
  if (humidity >= 61 and humidity <=70 ) {analogWrite(G,255);analogWrite(G,255);analogWrite(B,0);}
  if (humidity >= 71 and humidity <=80 ) {analogWrite(G,255);analogWrite(G,187);analogWrite(B,0);}
  if (humidity >= 81 and humidity <=90 ) {analogWrite(G,0);analogWrite(G,255);analogWrite(B,204);}

  u8g2.clearBuffer();                        //顯示前清除螢幕
  u8g2.setCursor(0, 5);                     //移動游標
  u8g2.print("智慧電子杜建平");           //寫入文字

  u8g2.setCursor(0, 25);                     //移動游標
  u8g2.print("溫度："+(String)temperature+" C");           //寫入文字

  u8g2.setCursor(0, 45);                    //移動游標
  u8g2.print("濕度：" + (String)humidity + " %");  //寫入文字

  //u8g2.drawLine(0, 11, 30, 11);  //劃線從0,11->30,11

  u8g2.sendBuffer();  //送到螢幕顯示

  delay(1000);
}
  
