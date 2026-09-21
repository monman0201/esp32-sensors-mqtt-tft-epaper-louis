# ESP32 實習專案總交接紀錄

## 目前接續版本

- 專案位置：`D:\monman\esp32\ESP32實習`
- 目前版本：`32_ili_mqtt_ctrl_page`
- 程式檔：`32_ili_mqtt_ctrl_page\32_ili_mqtt_ctrl_page.ino`
- 目前版本是在 `31_ili_mqtt_ctrl` 基礎上加入多頁 TFT 顯示、Gauge、歷史折線圖與 NTP 時間。
- 詳細版本紀錄另見：[handoff_32_ili_mqtt_ctrl_page.md](D:/monman/esp32/ESP32實習/handoff_32_ili_mqtt_ctrl_page.md)

## 專案功能總覽

ESP32 讀取 DHT11 溫度／濕度與光敏電阻亮度，使用 ILI9225 TFT 顯示環境資訊，並透過 MQTT 發布資料與接收 LED 控制命令。

目前功能包括：

- Wi-Fi 自動連線與斷線重連
- MQTT 3.1.1 匿名連線
- DHT11 溫度、濕度讀取
- 光敏電阻亮度讀取
- MQTT JSON 感測資料發布
- MQTT callback＋ArduinoJson LED 控制
- ILI9225 四頁顯示介面
- GPIO0 按鍵換頁
- NTP 台灣時區 UTC+8 時間同步
- 溫度、濕度、亮度最近 10 分鐘歷史折線圖

## 硬體接線

### ILI9225 TFT

- RST：GPIO26
- RS：GPIO25
- CS：GPIO16
- SDI/MOSI：GPIO23
- CLK/SCK：GPIO18
- 顯示尺寸：176×220

### 感測器與控制元件

- DHT11：GPIO14
- 光敏電阻：GPIO33（ADC）
- 綠色 LED：GPIO15
- 黃色 LED：GPIO2
- 紅色 LED：GPIO4
- 換頁按鍵：GPIO0，使用內建上拉，按下為 LOW

## Wi-Fi、NTP 與 MQTT 設定

- Wi-Fi SSID：`mon`
- Wi-Fi 密碼：`YOUR_WIFI_PASSWORD`
- MQTT Broker：`mqttgo.io`
- MQTT Port：`1883`
- 感測資料主題：`louis/class305/data`
- 發布週期：10 秒
- NTP：`pool.ntp.org`、`time.nist.gov`
- 時區：UTC+8

### 感測資料格式

```json
{"temp":25,"humi":65,"light":95}
```

### LED 控制主題

- 綠燈：`louis/class305/ctrl/gled`
- 黃燈：`louis/class305/ctrl/yled`
- 紅燈：`louis/class305/ctrl/rled`

命令格式：

```json
{"gled":"on"}
```

```json
{"yled":"off"}
```

```json
{"rled":"on"}
```

## 目前四頁介面

### P1/4：環境監控首頁

- 溫度、濕度、亮度資訊卡
- 濕度超過 65% 時閃爍警示
- Wi-Fi／MQTT 狀態列
- 台灣日期時間，格式如 `09/14 Mon 13:20`
- 日期時間每 10 秒更新

### P2/4：溫度

- 上半部：10～40°C Gauge
- 10～20°C 綠色、20～30°C 黃色、30～40°C 紅色
- 下半部：最近 10 分鐘溫度折線圖

### P3/4：濕度

- 上半部：0～100% 濕度 Gauge
- 0～40% 黃色、40～70% 綠色、70～100% 紅色
- 下半部：最近 10 分鐘濕度折線圖

### P4/4：亮度

- 上半部：0～100% 亮度 Gauge
- 0～30% 藍色、30～70% 黃色、70～100% 紅色
- 下半部：最近 10 分鐘亮度折線圖

所有歷史資料每 10 秒記錄一筆，最多保存 60 筆。折線圖使用圓點資料標記與細連線。

## 版本演進

- `27_mqtt_ctrl`：MQTT 控制與 LED 基礎版本，原本搭配 OLED。
- `28_ili9225`：ILI9225 TFT 儀表板基礎版本。
- `30_ili_mqtt`：ILI9225＋Wi-Fi＋MQTT 感測資料發布。
- `31_ili_mqtt_ctrl`：加入三個 LED MQTT 訂閱主題、callback 與 ArduinoJson。
- `32_ili_mqtt_ctrl_page`：加入 NTP、GPIO0 換頁、四頁顯示、Gauge 與三種歷史折線圖。

## 函式庫

專案使用：

- `TFT_22_ILI9225`
- `Adafruit_GFX`
- `SimpleDHT`
- `ArduinoJson`
- ESP32 內建 `WiFi`、`SPI`、`time`

函式庫位置：`D:\monman\esp32\ESP32實習\libraries`

## 編譯與燒錄

使用 Arduino CLI 與 ESP32 Dev Module 編譯。最新版本已成功編譯：

- Flash：約 59%
- RAM：約 14%

燒錄前先確認實際 COM 埠，不要直接假設為 COM4 或 COM7。燒錄後使用 115200 鮑率查看：

```text
ILI9225 MQTT dashboard started
WiFi connected, IP: 192.168.x.x
MQTT connected
MQTT subscribed:
```

## 驗證清單

- TFT 可正常顯示 P1/4
- GPIO0 可循環切換 P1/4～P4/4
- Wi-Fi 顯示 `WIFI:O`
- MQTT 顯示 `MQTT:O`
- 每 10 秒發布一筆感測 JSON
- 三個 LED MQTT 主題可控制對應 LED
- NTP 時間為台灣時間
- 四頁 Gauge 與折線圖持續保持上下分區
- 折線圖經過約 10 分鐘後逐步填滿

## 已知注意事項

- 最新程式修改後需重新燒錄，ESP32 才會使用最新版本。
- GPIO0 是 ESP32 開機模式腳位，燒錄或重置時避免持續按住按鍵。
- MQTT Broker 或網路不可用時，畫面可能顯示 MQTT 斷線，但感測顯示仍可運作。
- TFT 表面若有灰塵、刮痕或強烈反光，會影響展示照片的清晰度，建議實機展示前清潔螢幕並調整拍攝角度。

