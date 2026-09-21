# ESP32 實習專案總交接紀錄

## 紀錄日期

- 2026-09-14
- 本紀錄整合 `D:\monman\esp32\ESP32實習\handoff` 附檔中的歷史內容，以及本日對話完成的所有 ESP32 專案修改。

## 目前真正接續的版本

- 目前版本：`32_ili_mqtt_ctrl_page`
- 程式檔：`D:\monman\esp32\ESP32實習\32_ili_mqtt_ctrl_page\32_ili_mqtt_ctrl_page.ino`
- 編譯狀態：最新修改後已成功編譯
- 編譯容量：約 Flash 59%、RAM 14%
- 最新程式若尚未重新燒錄，ESP32 實機仍可能執行舊版本，需重新 upload。

舊交接紀錄：

- `D:\monman\esp32\ESP32實習\handoff`：早期 OLED／ThingSpeak／MQTT 紀錄
- `D:\monman\esp32\ESP32實習\handoff_30_ili_mqtt.md`
- `D:\monman\esp32\ESP32實習\handoff_32_ili_mqtt_ctrl_page.md`

## 專案演進

### 早期 OLED／ThingSpeak 階段

- `22_dht_ligt_oled`：DHT11、光敏電阻與 OLED 離線顯示。
- `23_dht_light_oled_thingspeak`：加入 Wi-Fi 與 ThingSpeak 上傳。
- ThingSpeak 欄位：
  - `field1`：溫度
  - `field2`：濕度
  - `field3`：亮度
- ThingSpeak 上傳週期：15 秒。
- 舊紀錄曾驗證 HTTP code `200`，並取得 entry id `7`。
- `23_dht_light_oled_thingspeak` 最後修改包含溫度 `C`、濕度 `%` 與亮度太陽圖示；該版最後一次修改後的編譯／上傳狀態需另外確認。

### MQTT／LED 控制階段

- `27_mqtt_ctrl`：OLED 顯示加上三顆 LED MQTT 控制。
- LED 接線：綠燈 GPIO15、黃燈 GPIO2、紅燈 GPIO4。
- 感測資料每 10 秒發布一次。
- 使用 MQTT callback 與 JSON 解析控制 LED。
- 舊紀錄曾成功燒錄至 COM7，並確認感測資料發布。

### ILI9225 階段

- `28_ili9225`：建立 ILI9225 TFT 環境監控儀表板。
- `30_ili_mqtt`：加入 Wi-Fi、MQTT 感測資料發布與 TFT 狀態列。
- `31_ili_mqtt_ctrl`：加入三個 LED MQTT 訂閱主題、callback 與 ArduinoJson。
- `32_ili_mqtt_ctrl_page`：目前版本，加入 NTP、GPIO0 換頁、四頁顯示、Gauge 與歷史折線圖。

## 硬體接線

### ESP32 與 ILI9225 TFT

- RST：GPIO26
- RS：GPIO25
- CS：GPIO16
- SDI／MOSI：GPIO23
- CLK／SCK：GPIO18
- TFT 尺寸：176×220

### 感測器、LED 與按鍵

- DHT11 資料線：GPIO14
- 光敏電阻：GPIO33（ADC）
- 綠色 LED：GPIO15
- 黃色 LED：GPIO2
- 紅色 LED：GPIO4
- 換頁按鍵：GPIO0
- GPIO0 使用 `INPUT_PULLUP`，按下時為 LOW。
- GPIO0 也是 ESP32 開機模式腳位，燒錄或重置時不要持續按住。

### 早期 OLED 接線（歷史版本）

- OLED SDA：GPIO21
- OLED SCL：GPIO22
- OLED 僅供 `22`、`23`、`27` 等舊版參考；目前 `32_ili_mqtt_ctrl_page` 使用 ILI9225，不使用 OLED。

## Wi-Fi、NTP 與 MQTT

### Wi-Fi

- SSID：`mon`
- 密碼：`YOUR_WIFI_PASSWORD`
- 斷線時會自動重連。

### NTP

- NTP Server：`pool.ntp.org`、`time.nist.gov`
- 台灣時區：UTC+8
- 第一頁最下方顯示格式：`09/14 Mon 13:20`
- 時間每 10 秒更新一次。

### MQTT

- Broker：`mqttgo.io`
- Port：`1883`
- MQTT 3.1.1
- 匿名連線
- Client ID 使用 ESP32 隨機值產生
- 感測資料主題：`louis/class305/data`
- 感測資料發布週期：10 秒

感測資料格式：

```json
{"temp":25,"humi":65,"light":95}
```

### LED 訂閱主題

目前採用附檔指定的三個獨立主題，不使用早期曾討論的單一 `mdow/class305/ctrl` 主題：

- 綠燈 GPIO15：`louis/class305/ctrl/gled`
- 黃燈 GPIO2：`louis/class305/ctrl/yled`
- 紅燈 GPIO4：`louis/class305/ctrl/rled`

控制格式：

```json
{"gled":"on"}
```

```json
{"yled":"off"}
```

```json
{"rled":"on"}
```

程式以 MQTT callback 收到完整 PUBLISH payload 後，使用 ArduinoJson 解析，並立即更新對應 GPIO。

## 目前四頁 TFT 介面

### P1/4：環境監控首頁

- 顯示溫度、濕度、亮度三張資訊卡。
- 濕度超過 65% 時，濕度數值閃爍紅色警示。
- 顯示 Wi-Fi／MQTT 狀態。
- 顯示台灣 NTP 日期時間。

### P2/4：溫度

- 上半部：溫度 Gauge。
- Gauge 範圍：10～40°C。
- 10～20°C：綠色。
- 20～30°C：黃色。
- 30～40°C：紅色。
- 下半部：最近 10 分鐘溫度折線圖。
- 每 10 秒記錄一筆，最多 60 筆。
- 資料點為圓點，連接線為細線。

### P3/4：濕度

- 上半部：濕度 Gauge。
- 範圍：0～100%。
- 0～40%：黃色，偏乾。
- 40～70%：綠色，舒適。
- 70～100%：紅色，偏濕。
- 下半部：最近 10 分鐘濕度折線圖。
- 每 10 秒記錄一筆，最多 60 筆。

### P4/4：亮度

- 上半部：亮度 Gauge。
- 範圍：0～100%。
- 0～30%：藍色，偏暗。
- 30～70%：黃色，中等。
- 70～100%：紅色，偏亮。
- 下半部：最近 10 分鐘亮度折線圖。
- 每 10 秒記錄一筆，最多 60 筆。

所有 Gauge 採半圓厚環、中央指針與白色分隔線設計。折線圖顯示 `-10m` 與 `NOW`，資料不足時顯示 `DATA n/60`。

## GPIO0 換頁機制

- GPIO0 使用硬體中斷 `FALLING` 偵測按下。
- 主迴圈執行防彈跳。
- 每按一次循環切換：P1/4 → P2/4 → P3/4 → P4/4 → P1/4。
- 已修正原本輪詢方式可能因 MQTT 封包等待而漏掉短按的問題。

## 今日對話完成的主要修改

1. 建立 `31_ili_mqtt_ctrl`，加入 LED MQTT callback 控制。
2. 依附檔將控制改為三個獨立 MQTT 主題。
3. 加入 ArduinoJson JSON 解析。
4. 加入 NTP UTC+8 與第一頁日期時間顯示。
5. 建立 `32_ili_mqtt_ctrl_page`，加入 IO0 換頁。
6. 新增溫度 Gauge 與最近 10 分鐘折線圖。
7. 依圖片範例調整 Gauge 為半圓厚環、分隔線與指針。
8. 修正日期時間過大造成換行，改為單行小字型。
9. 新增頁碼、資料收集狀態與折線圖網格線。
10. 折線圖改用圓點資料標記與細連線。
11. 新增第三頁濕度與第四頁亮度，兩頁皆採上下 Gauge＋折線圖配置。
12. 修正第四頁曾因舊函式週期重繪而只剩單一亮度直線圖的問題。
13. 目前最新版本已重新編譯成功。

## 使用函式庫

- `TFT_22_ILI9225`
- `Adafruit_GFX`
- `SimpleDHT`
- `ArduinoJson`
- ESP32 內建 `WiFi`
- ESP32 內建 `SPI`
- ESP32 內建 `time`

函式庫位置：`D:\monman\esp32\ESP32實習\libraries`

## 編譯與燒錄

Arduino CLI 路徑：

```powershell
C:\Users\user\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe
```

建議使用 ESP32 Dev Module 編譯：

```powershell
$cli='C:\Users\user\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$build='C:\Users\user\AppData\Local\Temp\esp32-build-32-ili-mqtt'
& $cli compile --fqbn esp32:esp32:esp32 `
  --libraries 'D:\monman\esp32\ESP32實習\libraries' `
  --build-path $build `
  --warnings all `
  'D:\monman\esp32\ESP32實習\32_ili_mqtt_ctrl_page'
```

燒錄前先確認實際 COM 埠，歷史上曾使用 COM4 與 COM7，不可直接假設：

```powershell
& $cli upload -p COM7 --fqbn esp32:esp32:esp32 --input-dir $build
```

若實際埠不是 COM7，請替換成目前裝置管理員顯示的 COM 埠。

## 燒錄後驗證

序列埠監控器使用 `115200` 鮑率，應能看到類似：

```text
ILI9225 MQTT dashboard started
WiFi connected, IP: 192.168.x.x
NTP time synchronization started (UTC+8)
MQTT connected
MQTT subscribed:
Published [louis/class305/data]: {"temp":25,"humi":65,"light":95}
```

驗證項目：

- TFT 顯示 P1/4。
- GPIO0 可循環切換四頁。
- 第一頁顯示日期時間。
- P2～P4 上下兩部分維持完整顯示。
- 每 10 秒產生感測歷史資料。
- MQTT 感測資料持續發布。
- 三個 LED 控制主題可分別控制 LED。
- 等待約 10 分鐘後，折線圖逐步填滿。

## 注意事項

- 最新程式若尚未重新燒錄，實機不會自動取得本日最後修改。
- GPIO0 為開機模式腳位，燒錄時避免按住。
- DHT11 讀取失敗時，溫度／濕度歷史資料不新增；亮度仍可繼續記錄。
- MQTT Broker 或 Wi-Fi 中斷時，程式會嘗試重連。
- TFT 表面若有灰塵、刮痕或強烈反光，會影響展示效果；實機展示前應清潔螢幕並調整拍攝角度。
- `23_dht_light_oled_thingspeak`、`27_mqtt_ctrl`、`30_ili_mqtt`、`31_ili_mqtt_ctrl` 為歷史版本，後續功能開發應以 `32_ili_mqtt_ctrl_page` 為基礎。

