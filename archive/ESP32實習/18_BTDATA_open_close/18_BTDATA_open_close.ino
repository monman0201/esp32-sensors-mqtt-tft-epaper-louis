#include <BluetoothSerial.h>
BluetoothSerial BT;
int redLED   = 0;  // 紅燈
int greenLED = 15;// 綠燈
int yellowLED = 2;// 黃燈

void setup() {
  Serial.begin(115200);
  BT.begin("杜建平");//請改名
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);

}

void loop() {
  //檢查序列監控視窗是否有輸入資料
  String Sdata="";
  String BTdata="";
  while (Serial.available()) {
    //讀取序列資料
    char Schar = Serial.read();//一次讀一個字元
    Sdata = Sdata + Schar;
  }

  if (Sdata!="") BT.println(Sdata);

  //檢查藍芽內是否有資料
  while (BT.available()) {
    //讀取藍芽資料
    char BTchar = BT.read();
    BTdata = BTdata + BTchar;
  }

  if (BTdata!="") Serial.println(BTdata);
//藍芽控制LED燈開關
  if(BTdata =="0")digitalWrite(redLED, LOW);
  if(BTdata =="1")digitalWrite(redLED, HIGH);
  if(BTdata =="2")digitalWrite(yellowLED, LOW);
  if(BTdata =="3")digitalWrite(yellowLED, HIGH);
  if(BTdata =="4")digitalWrite(greenLED, LOW);
  if(BTdata =="5")digitalWrite(greenLED, HIGH);
  if(BTdata =="6"){
  digitalWrite(redLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(greenLED, LOW);}
  if(BTdata =="7"){
  digitalWrite(redLED, HIGH);
  digitalWrite(yellowLED, HIGH);
  digitalWrite(greenLED, HIGH);}

  delay(10);

}

