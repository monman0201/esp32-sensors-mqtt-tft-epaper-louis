void setup() {
  // 初始化,只執行一次
Serial.begin(115200);//序列啟動,115200速率,功能為送出除錯訊息
}

void loop() {
  // 重複執行,無止無盡//1000ms=1s秒
Serial.println("father");
delay(1000);
Serial.println("mother");
delay(1000);
}

