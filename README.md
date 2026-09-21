# ESP32 Sensors MQTT TFT e-Paper

ESP32 物聯網環境監測與雲端控制實習專案。系統以 ESP32 為核心，整合環境感測、OLED／ILI9225 TFT 顯示、MQTT 雙向通訊、NTP 時間同步、歷史趨勢資料與三色 LED 遠端控制。

## 專案功能

- DHT11 讀取溫度與濕度
- 光敏電阻讀取環境亮度
- OLED 顯示即時感測資料
- ILI9225 TFT 四頁儀表板、Gauge 與歷史折線圖
- 透過 MQTT 發布感測資料並接收 LED 控制指令
- ThingSpeak、Google Sheets 與 LINE 通知整合成果整理
- Wi-Fi／MQTT 斷線重連與 NTP UTC+8 時間同步

## 硬體配置

| 元件 | GPIO／介面 | 用途 |
|---|---|---|
| DHT11 | GPIO14 | 溫度、濕度 |
| 光敏電阻 | GPIO33（ADC） | 環境亮度 |
| OLED | SDA GPIO21、SCL GPIO22 | 即時資訊顯示 |
| ILI9225 TFT | RST26、RS25、CS16、MOSI23、SCK18 | 儀表板與趨勢圖 |
| 綠／黃／紅 LED | GPIO15／GPIO2／GPIO4 | MQTT 遠端控制 |
| 換頁按鍵 | GPIO0，INPUT_PULLUP | 切換四個 TFT 頁面 |

## MQTT 設計

感測資料每 10 秒發布至：

```text
louis/class305/data
```

LED 控制主題：

```text
louis/class305/ctrl/gled
louis/class305/ctrl/yled
louis/class305/ctrl/rled
```

感測資料格式範例：

```json
{"temp":25,"humi":65,"light":95}
```

## 根目錄檔案

- `build_final_report.py`：產生整合版成果報告
- `final_integrated_report.py`：將整合內容加入既有報告
- `merge_report_0914.py`：合併 0914 版本學習成果
- `rebuild_report_0914_integrated.py`：重建整合後報告
- `rotate_figures_12_14.py`：調整報告圖片方向

這些腳本使用 `python-docx` 與 Pillow 產生或處理報告文件；執行前請依本機路徑調整腳本中的 `ROOT`、圖片與報告來源路徑。

## 注意事項

- 本 repository 不包含 Wi-Fi 密碼、雲端 API key、LINE token 或 MQTT 私密憑證；請使用本機設定或環境變數管理機密。
- `_waveshare_ePaper_tmp/` 是本機使用的 Waveshare 暫存／參考資料，約 2 GB，已透過 `.gitignore` 排除，未推送至 GitHub。
- 目前公開內容以 ESP32 實習成果文件產生工具與系統設計說明為主。

## 授權

本專案目前未指定正式開源授權；如需讓他人明確重製或修改，建議後續新增適合的 LICENSE。
