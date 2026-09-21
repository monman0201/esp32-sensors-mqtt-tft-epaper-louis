// ESP32 + HC-SR501 + LED (紅燈/綠燈)
const int sensorPin = 18;   // HC-SR501 OUT 腳位接到 GPIO18
const int redLedPin = 0;   // 紅燈接到 GPIO19
const int greenLedPin = 15; // 綠燈接到 GPIO15

void setup() {
  Serial.begin(115200);     
  pinMode(sensorPin, INPUT); 
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  // 預設狀態：紅燈熄滅，綠燈亮
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, HIGH);
}

void loop() {
  int sensorValue = digitalRead(sensorPin);

  if (sensorValue == HIGH) {
    Serial.println("1");            // 有偵測顯示 1
    digitalWrite(greenLedPin, LOW); // 熄滅綠燈
    digitalWrite(redLedPin, HIGH);  // 點亮紅燈
    delay(5000);                    // 保持亮 5 秒
    digitalWrite(redLedPin, LOW);   // 熄滅紅燈
  } else {
    Serial.println("0");            // 無偵測顯示 0
    digitalWrite(greenLedPin, HIGH);// 綠燈保持亮
  }

  delay(500); // 每 0.5 秒感應一次
}

