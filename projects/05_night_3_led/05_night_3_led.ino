const int sensorPin = 36;   // 光敏電阻輸入
const int led1 = 15;        // 第一顆燈
const int led2 = 2;         // 第二顆燈
const int led3 = 0;         // 第三顆燈

void setup() {
  Serial.begin(115200);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
}

void loop() {
  int rawValue = analogRead(sensorPin);  // 0 ~ 4095
  int mappedValue = 100 - (rawValue * 100 / 4095); // 映射到 100 ~ 0

  Serial.println(mappedValue);

  // 預設全部熄滅
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);

  // 判斷亮燈數量
  if (mappedValue <= 60) {
    digitalWrite(led1, HIGH);
  }
  if (mappedValue <= 40) {
    digitalWrite(led2, HIGH);
  }
  if (mappedValue <= 20) {
    digitalWrite(led3, HIGH);
  }

  delay(100); // 每 100ms 更新一次
}

