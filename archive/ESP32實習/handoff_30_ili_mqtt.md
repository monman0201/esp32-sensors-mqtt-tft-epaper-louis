# ESP32 ILI9225 MQTT 專案交接報告

## 專案位置

- 工作目錄：`D:\monman\esp32\ESP32實習`
- 目前程式：`30_ili_mqtt\30_ili_mqtt.ino`
- 基礎版本：`28_ili9225\28_ili9225.ino`
- 舊版 MQTT 參考：`27_mqtt_ctrl\27_mqtt_ctrl.ino`

## 專案目的

使用 ESP32、DHT11、光敏電阻及 ILI9225 TFT 顯示環境資料，並透過 MQTT 將溫度、濕度與亮度以 JSON 格式傳送至指定 Topic。

## 硬體接線

### ILI9225 TFT

- RST：GPIO26
- RS：GPIO25
- CS：GPIO16
- SDI/MOSI：GPIO23
- CLK/SCK：GPIO18
- 顯示尺寸：176×220

### 感測器

- DHT11 資料線：GPIO14
- 光敏電阻：GPIO33（ADC）

## Wi-Fi 設定

- SSID：`mon`
- 密碼：`YOUR_WIFI_PASSWORD`
- Wi-Fi 連線時會在 TFT 上顯示連線過程、SSID、成功狀態及 IP 位址。
- Wi-Fi 斷線時會自動重新連線。

## MQTT 設定

- Broker：`mqttgo.io`
- Port：`1883`
- Topic：`louis/class305/data`
- 更新週期：每 10 秒
- 連線方式：MQTT 3.1.1，匿名連線，Client ID 使用亂數產生。

### JSON 格式

目前實際傳送格式為：

```json
{"temp":25,"humi":65,"light":95}
```

- `temp`：溫度，攝氏
- `humi`：濕度，百分比
- `light`：亮度，0–100%

原始需求中的 `{"temp:25","humi":65,"light":95}` 將 `temp` 的冒號放錯位置；程式已修正為合法 JSON。

## TFT 畫面功能

- 頂端狀態列顯示：`WIFI:O  MQTT:O`
- `O`：連線正常
- `X`：連線錯誤或尚未連線
- 下方顯示三張卡片：
  - Temperature
  - Humidity
  - Brightness
- 濕度超過 65% 時，濕度數值會閃爍紅色警示。
- MQTT 連線過程會顯示 `MQTT CONNECTING`。

## 已完成事項

- 已建立 `30_ili_mqtt\30_ili_mqtt.ino`。
- 已加入 Wi-Fi 連線及自動重連。
- 已加入 MQTT 連線、Keep Alive 及發布功能。
- 已加入 JSON 格式感測資料。
- 已加入 TFT Wi-Fi/MQTT 狀態列。
- 已保留原本 ILI9225 dashboard 與濕度警示功能。
- 已使用 Arduino CLI 編譯成功。

## 編譯結果

編譯指令使用 ESP32 Dev Module：

```powershell
$cli='C:\Users\user\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$build='C:\Users\user\AppData\Local\Temp\esp32-build-30-ili-mqtt-3'
& $cli compile --fqbn esp32:esp32:esp32 `
  --libraries 'D:\monman\esp32\ESP32實習\libraries' `
  --build-path $build `
  --warnings all `
  'D:\monman\esp32\ESP32實習\30_ili_mqtt'
```

最後一次編譯結果：

- Flash：約 57%
- RAM：約 13%
- 編譯成功
- 尚未燒錄至 ESP32

## 下一步：燒錄

燒錄前確認 ESP32 的 COM 埠。先前其他版本曾使用 COM4 或 COM7，不能直接假設目前仍相同。

```powershell
$cli='C:\Users\user\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$build='C:\Users\user\AppData\Local\Temp\esp32-build-30-ili-mqtt-3'
& $cli upload -p COM7 --fqbn esp32:esp32:esp32 --input-dir $build
```

若目前不是 COM7，請改成實際使用的 COM 埠。

## 燒錄後驗證

使用序列埠監控器，鮑率 `115200`，應看到類似：

```text
ILI9225 MQTT dashboard started
WiFi connecting...
WiFi connected, IP: 192.168.x.x
MQTT connected
Published [louis/class305/data]: {"temp":25,"humi":65,"light":95}
```

每 10 秒應發布一筆 JSON。TFT 頂端應顯示：

```text
WIFI:O  MQTT:O
```

## 注意事項

- `30_ili_mqtt` 使用專案內已存在的 `TFT_22_ILI9225`、`Adafruit_GFX` 及 `SimpleDHT` 函式庫。
- 程式使用原生 MQTT 封包，不需要另外安裝 `PubSubClient`。
- 若 Wi-Fi 正常但 MQTT 顯示 `X`，先確認 `mqttgo.io:1883` 是否可連線。
- 若 DHT11 讀取失敗，畫面會顯示 `ERROR`，且該次不發布感測資料。
- `handoff` 是較早期的總交接紀錄；本檔案以 ILI9225 MQTT 版本為目前接續依據。

