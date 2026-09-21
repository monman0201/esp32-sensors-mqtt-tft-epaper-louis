#include <SimpleDHT.h>

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = 19;//我有一個整數，我把它命名為pinDHT11，預設值為19
SimpleDHT11 dht11(pinDHT11);//我有一支SimpleDHT11規格的溫溼度計，我把它命名為dht11，(放在19腳)
int greenled = 15;        // 綠燈在15腳
int redled = 0;         // 紅燈在0腳
int blueled = 2;         // 藍燈在2腳
int yellowled = 16;         // 黃燈在16腳

void setup() {
  Serial.begin(115200);
  pinMode(greenled, OUTPUT);
  pinMode(redled, OUTPUT);
  pinMode(blueled, OUTPUT);
  pinMode(yellowled, OUTPUT);
}

void loop() {
  // start working...
  Serial.println("=================================");//println的ln=line,換行
  Serial.println("Sample DHT11...");
  
  // byte=>0~255範圍的整數，int=>20億左右
  byte temperature = 0;
  byte humidity = 0;
  //如果讀取錯誤，就印出錯誤訊息並返回return，返回開始的地方
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    return;
  }
  
  Serial.println("Sample OK:" + (String)temperature + " *C, " +((String)humidity) + " H");
  //(int)強制轉型為整數
  // DHT11 sampling rate is 1HZ.

  digitalWrite(greenled, LOW);
  digitalWrite(redled, LOW);
  digitalWrite(blueled, LOW);
  digitalWrite(yellowled, LOW);
  if (humidity >= 70) {digitalWrite(redled, HIGH);}
  else{digitalWrite(greenled, HIGH);}  

  if (temperature >= 30) {digitalWrite(yellowled, HIGH);}
  else{digitalWrite(blueled, HIGH);}  
  delay(1500);
}

