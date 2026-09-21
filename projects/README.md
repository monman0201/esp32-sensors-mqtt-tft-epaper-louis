# ESP32 範例專案

每個資料夾都是一個可在 Arduino IDE 開啟的 ESP32 範例；資料夾名稱與 `.ino` 檔名一致。需要 Wi‑Fi、雲端或 MQTT 的範例，已將公開 repository 中的憑證改為 placeholder，請先填入自己的設定。

| 專案 | 功能 |
|---|---|
| `01_hello` | Serial／基本輸出入門 |
| `02_RGLED` | RGB LED 基本控制 |
| `03_nightled` | 夜燈與光線控制 |
| `04_PIR` | PIR 人體感測 |
| `05_night_3_led` | 夜燈與三色 LED |
| `06_aht` | AHT 溫濕度感測 |
| `07_i2c` | I²C 介面掃描與測試 |
| `08_OLED` | OLED 顯示入門 |
| `12_analogWrite_GLED` | PWM 控制綠色 LED |
| `14_DHT_OLED_RGBLED` | DHT11、OLED 與 RGB LED 整合 |
| `15_buzzer` | 蜂鳴器輸出 |
| `16_buzzer_alarm` | 蜂鳴器警報邏輯 |
| `17_sonic` | 超音波距離感測 |
| `18_BTDATA_open_close` | Bluetooth 開關控制 |
| `19_PM2_5_OLED` | PM2.5 感測與 OLED 顯示 |
| `19_PM2_5` | PM2.5 感測與網路資料 |
| `21_weather_OLED` | 天氣資料與 OLED 顯示 |
| `22_dht_light_oled` | DHT11、光敏電阻與 OLED |
| `23_dht_light_oled_thingspeak` | 感測資料上傳 ThingSpeak |
| `24_google_dht_light_oled` | 感測資料寫入 Google Sheets |
| `25_dht_line` | LINE 異常通知 |
| `26_mqtt` | MQTT 感測資料發布 |
| `27_mqtt_ctrl` | MQTT 感測資料與 LED 控制 |
| `28_ili9225` | ILI9225 TFT 顯示入門 |
| `29_ili9225_weather` | TFT 天氣儀表板 |
| `30_ili_mqtt` | TFT 與 MQTT 整合 |
| `31_ili_mqtt_ctrl` | TFT、MQTT 與 LED 控制 |
| `32_ili_mqtt_ctrl_page` | 四頁 TFT 儀表板、MQTT、NTP 與趨勢圖 |
| `33_epaper` | e-Paper 顯示入門 |
| `34_epaper_dht` | e-Paper 與 DHT11 |
| `35_epaper_mqtt` | e-Paper、DHT11、MQTT 與 LED 控制 |

## 使用方式

1. 在 Arduino IDE 開啟對應資料夾內的 `.ino` 檔案。
2. 選擇 ESP32 開發板與正確的序列埠。
3. 安裝程式使用到的 Arduino libraries。
4. 對含網路功能的範例，先把 `YOUR_WIFI_SSID`、`YOUR_WIFI_PASSWORD` 等 placeholder 改成自己的設定。
5. 編譯、上傳並開啟 Serial Monitor 觀察結果。
