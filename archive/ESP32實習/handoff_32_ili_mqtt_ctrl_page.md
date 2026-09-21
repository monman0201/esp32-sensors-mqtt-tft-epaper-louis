# ESP32 ILI9225 MQTT 四頁版交接紀錄

## 更新日期

- 2026-09-14

## 目前程式

- 專案目錄：`D:\monman\esp32\ESP32實習\32_ili_mqtt_ctrl_page`
- 程式檔：`32_ili_mqtt_ctrl_page\32_ili_mqtt_ctrl_page.ino`
- 基礎版本：由 `31_ili_mqtt_ctrl` 延伸
- 顯示器：ILI9225，176×220

## 硬體接線

### ILI9225 TFT

- RST：GPIO26
- RS：GPIO25
- CS：GPIO16
- SDI/MOSI：GPIO23
- CLK/SCK：GPIO18

### 感測器與按鍵

- DHT11：GPIO14
- 光敏電阻：GPIO33（ADC）
- 綠色 LED：GPIO15
- 黃色 LED：GPIO2
- 紅色 LED：GPIO4
- 換頁按鍵：GPIO0，使用 `INPUT_PULLUP`，按下為 LOW

## Wi-Fi、NTP 與 MQTT

- Wi-Fi SSID：`mon`
- Wi-Fi 密碼：`YOUR_WIFI_PASSWORD`
- MQTT Broker：`mqttgo.io`
- MQTT Port：`1883`
- 感測資料主題：`louis/class305/data`
- 感測資料發布週期：10 秒
- 時間同步：NTP
- 台灣時區：UTC+8
- NTP Server：`pool.ntp.org`、`time.nist.gov`

### LED 訂閱主題

- 綠燈：`louis/class305/ctrl/gled`
- 黃燈：`louis/class305/ctrl/yled`
- 紅燈：`louis/class305/ctrl/rled`

使用 ArduinoJson 解析 MQTT callback：

```json
{"gled":"on"}
```

```json
{"yled":"off"}
```

```json
{"rled":"on"}
```

## 四頁畫面

### 第 1 頁：環境監控首頁

- 顯示溫度、濕度、亮度三張資訊卡
- 濕度超過 65% 時閃爍警示
- 顯示 Wi-Fi、MQTT 狀態
- 顯示台灣時間，格式如：`09/14 Mon 13:20`
- 日期時間每 10 秒更新

### 第 2 頁：溫度趨勢

- 上半部：溫度 Gauge
- 溫度範圍：10～40°C
- 10～20°C：綠色
- 20～30°C：黃色
- 30～40°C：紅色
- 下半部：最近 10 分鐘溫度折線圖
- 每 10 秒記錄一筆，最多 60 筆
- 溫度資料點使用圓點，連線使用細線

### 第 3 頁：濕度趨勢

- 上半部：濕度 Gauge，範圍 0～100%
- 0～40%：黃色，偏乾
- 40～70%：綠色，舒適
- 70～100%：紅色，偏濕
- 下半部：最近 10 分鐘濕度折線圖
- 每 10 秒記錄一筆，最多 60 筆

### 第 4 頁：亮度趨勢

- 上半部：亮度 Gauge，範圍 0～100%
- 0～30%：藍色，偏暗
- 30～70%：黃色，中等
- 70～100%：紅色，偏亮
- 下半部：最近 10 分鐘亮度折線圖
- 每 10 秒記錄一筆，最多 60 筆

## 換頁功能

- GPIO0 使用中斷 `FALLING` 偵測按下事件
- 主迴圈執行防彈跳
- IO0 按鍵循環切換 `P1/4`、`P2/4`、`P3/4`、`P4/4`
- 已修正短按可能被 MQTT 等待流程漏掉的問題

## 最近修正問題

- 修正第四頁亮度畫面會先顯示上下兩部分、之後只剩單一亮度直線圖的問題。
- 原因是週期更新仍呼叫舊版 `drawBrightnessDetail()`，已改為呼叫目前頁面的完整繪圖函式。
- Gauge 已依照半圓厚環、中央指針與分隔線的視覺範例調整。
- 折線圖資料點改為圓點，連線維持細線。
- 加入頁碼、資料收集狀態、`-10m`／`NOW` 時間軸標示與較清楚的網格線。

## 編譯結果

使用 ESP32 Dev Module 編譯成功：

- Flash：約 59%
- RAM：約 14%
- ArduinoJson、SimpleDHT、Adafruit_GFX、TFT_22_ILI9225 均可正常編譯

最新程式在最近修改後已重新編譯成功；若要使用最新四頁版與第四頁修正版，需重新燒錄至 ESP32。

## 後續建議

- 燒錄最新 `32_ili_mqtt_ctrl_page.ino`
- 重新確認 GPIO0 按鍵可循環切換四頁
- 等待約 10 分鐘，確認溫度、濕度、亮度三種折線圖逐步填滿
- 確認 MQTT 三個 LED 控制主題與實際 LED 狀態一致
- 展示時清潔 TFT 表面並調整拍攝角度，降低反光與刮痕對畫面的影響

