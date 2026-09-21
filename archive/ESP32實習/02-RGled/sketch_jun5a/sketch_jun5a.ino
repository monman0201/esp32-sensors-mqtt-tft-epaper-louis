// LED 腳位設定
int greenLED = 15;  // 綠燈
int blueLED  = 16;  // 藍燈
int redLED   = 19;  // 紅燈

void setup() {
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
}

void loop() {

  // 綠燈亮 3 秒
  digitalWrite(greenLED, HIGH);
  delay(3000);
  digitalWrite(greenLED, LOW);

  // 藍燈閃爍 3 次
  for (int i = 0; i < 3; i++) {

    digitalWrite(blueLED, HIGH);
    delay(500);

    digitalWrite(blueLED, LOW);
    delay(500);
  }

  // 紅燈亮 5 秒
  digitalWrite(redLED, HIGH);
  delay(5000);
  digitalWrite(redLED, LOW);
}
