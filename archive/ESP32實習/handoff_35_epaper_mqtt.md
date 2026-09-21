# ESP32 電子紙 MQTT 專案交接報告

## 更新日期

- 2026-09-21

## 目前版本

- 專案目錄：`D:\monman\esp32\ESP32實習\35_epaper_mqtt`
- 程式檔：`35_epaper_mqtt\35_epaper_mqtt.ino`
- 基礎版本：由 `34_epaper_dht` 延伸
- `33_epaper`、`34_epaper_dht` 均保留，未被覆蓋

## 電子紙硬體

- 型號：Waveshare 2.9 吋三色電子紙，296×128，V4 模組
- 驅動版：Rev2.1
- DIN／MOSI：GPIO23
- CLK／SCK：GPIO18
- CS：GPIO27
- DC：GPIO26
- RST：GPIO25
- BUSY：GPIO34

## 感測器與燈號

- DHT11：GPIO14
- 光敏電阻：GPIO33（ADC）
- 綠色燈：GPIO15
- 黃色燈：GPIO2
- 紅色燈：GPIO4

## 電子紙畫面

- 紅色標題列：`ENV MONITOR`
- 三張資訊卡：TEMP、HUMI、LIGHT
- 溫度計、水滴、太陽 icon 使用黑色外框／紅色細節
- 數值使用黑色顯示
- 電子紙每 60 秒完整更新一次
- DHT11 感測值每 2 秒讀取一次，避免過度讀取

## Wi-Fi 與 MQTT 設定

- Wi-Fi SSID：`mon`
- Wi-Fi 密碼：`YOUR_WIFI_PASSWORD`
- MQTT Broker：`mqttgo.io`
- MQTT Port：`1883`
- 感測資料 Topic：`louis/class305/data`
- 感測資料格式：

```json
{"temp":25,"humi":60,"light":75}
```

### 燈號控制 Topic

- 綠燈：`louis/class305/ctrl/gled`
- 黃燈：`louis/class305/ctrl/yled`
- 紅燈：`louis/class305/ctrl/rled`

控制訊息範例：

```json
{"gled":"on"}
```

```json
{"yled":"off"}
```

```json
{"rled":"on"}
```

## MQTT 穩定性修正

- MQTT 感測資料每 10 秒發布一次
- MQTT 重新連線間隔 5 秒，避免快速重複連線
- Wi-Fi 啟用自動重連
- 關閉 Wi-Fi sleep，降低長時間運作斷線機率
- MQTT 每 20 秒送出 PINGREQ
- 增加 PINGRESP 檢查，逾時後自動重新連線
- 發布資料後檢查連線是否仍存在

## 電子紙 BUSY 斷線問題

曾從序列埠觀察到：

- Wi-Fi 可成功連線並取得 IP
- MQTT 可成功連線
- 感測資料可成功發布
- 三個燈號控制訊息可成功收到
- 長時間運作後，主迴圈可能停止，造成 MQTT 無法傳輸與控制

檢查 Waveshare 驅動後發現 `ReadBusy()` 原本使用無限迴圈等待 BUSY 腳位，若電子紙 BUSY 沒有正常釋放，ESP32 會永久卡在電子紙更新流程。

已修改：

- 驅動檔：`libraries\Waveshare_epd2in9b_V4_ESP32\epd2in9b_V4.cpp`
- BUSY 等待增加 3 秒逾時保護
- 正常的 `e-Paper busy release` 訊息已移除
- 只有 BUSY 逾時時顯示：

```text
e-Paper BUSY timeout; MQTT loop continues
```

## 序列埠設定

- 程式目前使用：`Serial.begin(115200)`
- 序列監控器設定：`115200 baud，8-N-1`
- 先前曾測試 9600 baud，後已改回 115200
- ESP32 開機最前面的 ROM 訊息若速率不一致，可能短暫顯示亂碼；進入使用者程式後應使用 115200 讀取

## 最近一次燒錄狀態

- 最新程式已成功編譯
- 最新程式已成功上傳至 COM9
- 上傳後會自動重置 ESP32
- 最新版本包含：MQTT 重連、Wi-Fi 自動重連、DHT11 讀取間隔、MQTT 心跳檢查與電子紙 BUSY 逾時保護

## 後續檢查建議

1. 開啟序列監控器，設定 115200 baud。
2. 確認啟動後依序看到 Wi-Fi connected、MQTT connected。
3. 觀察 60 秒電子紙更新時是否出現 BUSY timeout。
4. 若仍斷線，確認電子紙 BUSY 線、3.3V 供電與 GND 是否穩定。
5. 若出現 BUSY timeout 但 MQTT 持續發布，表示主迴圈未被電子紙卡住；若 MQTT 仍中斷，需再檢查 Wi-Fi 訊號或 MQTT Broker 狀態。

