#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>

// 避免舊版 ILI9225 library 與 Adafruit_GFX
// 發生 GFXfont / GFXglyph 重複宣告
#define _GFFFONT_H_
#include <TFT_22_ILI9225.h>

// =====================================================
// ILI9225 接線
// =====================================================
const int TFT_RST = 26;
const int TFT_RS  = 25;
const int TFT_CS  = 16;
const int TFT_SDI = 23;
const int TFT_CLK = 18;

// =====================================================
// WiFi
// =====================================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// =====================================================
// Open-Meteo API
//
// 高雄：
// latitude  = 22.6273
// longitude = 120.3014
//
// 注意：
// temperature_2m_minx 是錯誤參數
// 正確是 temperature_2m_min
//
// 另外增加 precipitation_probability_max
// =====================================================
const char* WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast"
  "?latitude=22.6273"
  "&longitude=120.3014"
  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max"
  "&timezone=Asia%2FTaipei"
  "&forecast_days=7";

// 每 60 秒更新一次
const unsigned long UPDATE_INTERVAL = 60000UL;

// =====================================================
// TFT
// =====================================================
TFT_22_ILI9225 tft(
  TFT_RST,
  TFT_RS,
  TFT_CS,
  -1
);

// =====================================================
// Adafruit_GFX 顯示介面
// =====================================================
class ILI9225_GFX : public Adafruit_GFX {

public:

  ILI9225_GFX()
    : Adafruit_GFX(176, 220) {
  }

  void drawPixel(
    int16_t x,
    int16_t y,
    uint16_t color
  ) override {

    if (
      x >= 0 &&
      x < 176 &&
      y >= 0 &&
      y < 220
    ) {

      tft.drawPixel(
        x,
        y,
        color
      );
    }
  }
};

ILI9225_GFX gfx;

// =====================================================
// HTTPS Client
// =====================================================
WiFiClientSecure secureClient;

// =====================================================
// 系統狀態
// =====================================================
unsigned long lastUpdate = 0;

String lastFetchError = "";

// =====================================================
// 顏色
// =====================================================
const uint16_t BG       = COLOR_BLACK;
const uint16_t WHITE_C  = COLOR_WHITE;
const uint16_t CYAN_C   = COLOR_CYAN;
const uint16_t YELLOW_C = COLOR_YELLOW;
const uint16_t BLUE_C   = 0x5D9F;

// =====================================================
// Weather Code → 英文
//
// Open-Meteo WMO Weather Code
// =====================================================
const char* weatherText(
  int code
) {

  if (code == 0)
    return "SUN";

  if (code <= 3)
    return "CLOUD";

  if (
    code == 45 ||
    code == 48
  )
    return "FOG";

  if (
    code >= 51 &&
    code <= 67
  )
    return "RAIN";

  if (
    code >= 71 &&
    code <= 77
  )
    return "SNOW";

  if (
    code >= 80 &&
    code <= 82
  )
    return "SHOWR";

  if (code >= 95)
    return "STORM";

  return "OTHER";
}

// =====================================================
// Weather Icon - Compact 16x18
// =====================================================
// 小螢幕專用：圖示固定縮小，避免與日期/文字重疊
void drawWeatherIcon(
  int16_t x,
  int16_t y,
  int code
) {

  // ===================================================
  // Sunny - 小太陽
  // ===================================================
  if (code == 0) {

    gfx.drawCircle(
      x + 8,
      y + 7,
      4,
      YELLOW_C
    );

    // 8 根短光線
    gfx.drawLine(x + 8, y + 0,  x + 8, y + 2,  YELLOW_C);
    gfx.drawLine(x + 8, y + 12, x + 8, y + 14, YELLOW_C);
    gfx.drawLine(x + 1, y + 7,  x + 3, y + 7,  YELLOW_C);
    gfx.drawLine(x + 13, y + 7, x + 15, y + 7, YELLOW_C);
    gfx.drawLine(x + 3, y + 2,  x + 4, y + 4,  YELLOW_C);
    gfx.drawLine(x + 12, y + 10, x + 13, y + 12, YELLOW_C);
    gfx.drawLine(x + 3, y + 12, x + 4, y + 10, YELLOW_C);
    gfx.drawLine(x + 12, y + 4,  x + 13, y + 2,  YELLOW_C);
  }

  // ===================================================
  // Rain / Showers - 小雨滴
  // ===================================================
  else if (
    code >= 51 &&
    code <= 82
  ) {

    gfx.drawCircle(
      x + 8,
      y + 6,
      4,
      BLUE_C
    );

    // 三條短雨線，控制在 16x18 範圍內
    gfx.drawLine(x + 3,  y + 12, x + 2,  y + 16, CYAN_C);
    gfx.drawLine(x + 8,  y + 12, x + 7,  y + 16, CYAN_C);
    gfx.drawLine(x + 13, y + 12, x + 12, y + 16, CYAN_C);
  }

  // ===================================================
  // Cloud / Fog / Storm - 小雲朵
  // ===================================================
  else {

    gfx.drawCircle(
      x + 4,
      y + 10,
      3,
      WHITE_C
    );

    gfx.drawCircle(
      x + 8,
      y + 7,
      4,
      WHITE_C
    );

    gfx.drawCircle(
      x + 13,
      y + 10,
      3,
      WHITE_C
    );

    gfx.drawLine(
      x + 2,
      y + 13,
      x + 15,
      y + 13,
      WHITE_C
    );

    // 暴雨/雷雨：加入小閃電，方便辨識 STORM
    if (code >= 95) {
      gfx.drawLine(x + 9,  y + 14, x + 7,  y + 17, YELLOW_C);
      gfx.drawLine(x + 7,  y + 17, x + 9,  y + 17, YELLOW_C);
      gfx.drawLine(x + 9,  y + 17, x + 8,  y + 19, YELLOW_C);
    }
  }
}

// =====================================================
// TFT Header
// =====================================================
void drawHeader(
  const char* status
) {

  tft.setBackgroundColor(
    BG
  );

  tft.clear();

  gfx.setTextSize(1);

  // ---------------------------------------------------
  // Header 背景
  // ---------------------------------------------------
  gfx.fillRect(
    2,
    2,
    172,
    24,
    0x0841
  );

  // ---------------------------------------------------
  // 標題
  // ---------------------------------------------------
  gfx.setTextColor(
    CYAN_C
  );

  gfx.setCursor(
    18,
    5
  );

  gfx.print(
    "KAOHSIUNG 7-DAY WEATHER"
  );

  // ---------------------------------------------------
  // 狀態
  // ---------------------------------------------------
  gfx.setTextColor(
    WHITE_C
  );

  gfx.setCursor(
    18,
    15
  );

  gfx.print(
    status
  );

  // ---------------------------------------------------
  // 分隔線
  // 注意：不要畫在文字高度範圍內，避免與 status 重疊
  // ---------------------------------------------------
  gfx.drawLine(
    3,
    28,
    172,
    28,
    CYAN_C
  );
}

// =====================================================
// 顯示 7 天預報
// =====================================================
void drawForecast(
  JsonObject daily
) {

  drawHeader(
    "Updated / Open-Meteo"
  );

  // ---------------------------------------------------
  // 取得 JSON 陣列
  // ---------------------------------------------------
  JsonArray dates =
    daily["time"].as<JsonArray>();

  JsonArray codes =
    daily["weather_code"].as<JsonArray>();

  JsonArray highs =
    daily["temperature_2m_max"].as<JsonArray>();

  JsonArray lows =
    daily["temperature_2m_min"].as<JsonArray>();

  JsonArray rains =
    daily[
      "precipitation_probability_max"
    ].as<JsonArray>();

  gfx.setTextSize(1);

  // ---------------------------------------------------
  // 7 天
  // ---------------------------------------------------
  for (
    uint8_t i = 0;
    i < 7;
    i++
  ) {

    // 每一天 26 pixel
    // Header 結束於 y=28，因此第一列從 y=30 開始
    int16_t y =
      30 + i * 26;

    // -------------------------------------------------
    // Date
    // -------------------------------------------------
    const char* date =
      dates[i] | "----";

    int code =
      codes[i] | -1;

    // -------------------------------------------------
    // MM/DD
    // -------------------------------------------------
    char dateShort[6] =
      "--/--";

    if (
      strlen(date) >= 10
    ) {

      dateShort[0] =
        date[5];

      dateShort[1] =
        date[6];

      dateShort[2] =
        '/';

      dateShort[3] =
        date[8];

      dateShort[4] =
        date[9];

      dateShort[5] =
        '\0';
    }

    // -------------------------------------------------
    // Temperature
    // -------------------------------------------------
    int highTemp =
      highs[i] | 0;

    int lowTemp =
      lows[i] | 0;

    char temp[18];

    snprintf(
      temp,
      sizeof(temp),
      "%d/%dC",
      highTemp,
      lowTemp
    );

    // -------------------------------------------------
    // Rain probability
    // -------------------------------------------------
    int rainProbability = 0;

    if (
      !rains.isNull() &&
      i < rains.size()
    ) {

      rainProbability =
        rains[i] | 0;
    }

    char rain[8];

    snprintf(
      rain,
      sizeof(rain),
      "%d%%",
      rainProbability
    );

    // -------------------------------------------------
    // 美化：交錯背景 + 每列底部分隔線
    // -------------------------------------------------
    if (
      i % 2 == 0
    ) {

      gfx.fillRect(
        3,
        y,
        170,
        24,
        0x0841
      );
    }

    gfx.drawLine(
      5,
      y + 24,
      170,
      y + 24,
      0x3186
    );

    // -------------------------------------------------
    // Date
    // -------------------------------------------------
    gfx.setTextColor(
      WHITE_C
    );

    gfx.setCursor(
      4,
      y + 8
    );

    gfx.print(
      dateShort
    );

    // -------------------------------------------------
    // Weather Icon
    // -------------------------------------------------
    drawWeatherIcon(
      39,
      y + 2,
      code
    );

    // -------------------------------------------------
    // Weather Text
    // -------------------------------------------------
    gfx.setTextColor(
      YELLOW_C
    );

    gfx.setCursor(
      59,
      y + 8
    );

    gfx.print(
      weatherText(code)
    );

    // -------------------------------------------------
    // Temperature
    // -------------------------------------------------
    gfx.setTextColor(
      WHITE_C
    );

    gfx.setCursor(
      112,
      y + 8
    );

    gfx.print(
      temp
    );

    // -------------------------------------------------
    // Rain
    // -------------------------------------------------
    gfx.setTextColor(
      CYAN_C
    );

    gfx.setCursor(
      153,
      y + 8
    );

    gfx.print(
      rain
    );
  }
}

// =====================================================
// 顯示錯誤
// =====================================================
void showError(
  const char* message
) {

  drawHeader(
    "Weather download failed"
  );

  gfx.setTextColor(
    COLOR_RED
  );

  gfx.setCursor(
    8,
    70
  );

  gfx.print(
    message
  );

  gfx.setTextColor(
    WHITE_C
  );

  gfx.setCursor(
    8,
    90
  );

  gfx.print(
    "Retry in 60 seconds"
  );
}

// =====================================================
// 取得天氣資料
// =====================================================
bool fetchWeather() {

  // ===================================================
  // 檢查 WiFi
  // ===================================================
  if (
    WiFi.status() != WL_CONNECTED
  ) {

    lastFetchError =
      "WiFi disconnected";

    Serial.println(
      lastFetchError
    );

    return false;
  }

  // ===================================================
  // HTTPS
  // ===================================================
  secureClient.setInsecure();

  HTTPClient http;

  http.setTimeout(
    20000
  );

  // ---------------------------------------------------
  // 使用 HTTP/1.0
  // 避免某些 ESP32 HTTPS response / chunk 問題
  // ---------------------------------------------------
  http.useHTTP10(
    true
  );

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "Connecting to Open-Meteo..."
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "URL:"
  );

  Serial.println(
    WEATHER_URL
  );

  // ===================================================
  // HTTP Begin
  // ===================================================
  if (
    !http.begin(
      secureClient,
      WEATHER_URL
    )
  ) {

    lastFetchError =
      "HTTPS begin failed";

    Serial.println(
      "HTTP begin failed"
    );

    return false;
  }

  // ===================================================
  // HTTP Header
  // ===================================================
  http.addHeader(
    "Accept",
    "application/json"
  );

  http.addHeader(
    "Accept-Encoding",
    "identity"
  );

  // ===================================================
  // GET
  // ===================================================
  int httpCode =
    http.GET();

  Serial.printf(
    "Weather HTTP code: %d\n",
    httpCode
  );

  // ===================================================
  // HTTP 錯誤
  // ===================================================
  if (
    httpCode != HTTP_CODE_OK
  ) {

    lastFetchError =
      "HTTP " +
      String(httpCode);

    Serial.print(
      "Weather error: "
    );

    Serial.println(
      lastFetchError
    );

    // 讀取 API 錯誤內容
    String response =
      http.getString();

    Serial.println();
    Serial.println(
      "API error response:"
    );

    Serial.println(
      response
    );

    http.end();

    return false;
  }

  // ===================================================
  // 取得完整 JSON
  //
  // 不再使用：
  //
  // deserializeJson(doc, http.getStream());
  //
  // 改成完整讀取後再解析
  // ===================================================
  String payload =
    http.getString();

  http.end();

  // ===================================================
  // 顯示 Response Size
  // ===================================================
  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "Open-Meteo response length:"
  );

  Serial.println(
    payload.length()
  );

  Serial.println(
    "================================"
  );

  // ===================================================
  // 空資料
  // ===================================================
  if (
    payload.length() == 0
  ) {

    lastFetchError =
      "Empty API response";

    Serial.println(
      "ERROR: API response is empty"
    );

    return false;
  }

  // ===================================================
  // 印出 API Response
  // ===================================================
  Serial.println();
  Serial.println(
    "API response:"
  );

  if (
    payload.length() > 1500
  ) {

    Serial.println(
      payload.substring(
        0,
        1500
      )
    );

    Serial.println(
      "... response truncated ..."
    );

  } else {

    Serial.println(
      payload
    );
  }

  // ===================================================
  // ArduinoJson
  // ===================================================
  DynamicJsonDocument doc(
    16384
  );

  DeserializationError error =
    deserializeJson(
      doc,
      payload
    );

  // ===================================================
  // JSON 解析錯誤
  // ===================================================
  if (error) {

    lastFetchError =
      String("JSON ") +
      error.c_str();

    Serial.println();
    Serial.println(
      "================================"
    );

    Serial.print(
      "Weather JSON error: "
    );

    Serial.println(
      error.c_str()
    );

    Serial.println(
      "================================"
    );

    Serial.println(
      "First 300 characters:"
    );

    Serial.println(
      payload.substring(
        0,
        min(
          300,
          (int)payload.length()
        )
      )
    );

    return false;
  }

  // ===================================================
  // 取得 daily
  // ===================================================
  JsonObject daily =
    doc["daily"].as<JsonObject>();

  if (
    daily.isNull()
  ) {

    lastFetchError =
      "daily object missing";

    Serial.println(
      "ERROR: daily object missing"
    );

    return false;
  }

  // ===================================================
  // time
  // ===================================================
  JsonArray dates =
    daily["time"].as<JsonArray>();

  if (
    dates.isNull() ||
    dates.size() < 7
  ) {

    lastFetchError =
      "daily time data incomplete";

    Serial.println(
      "ERROR: daily time data incomplete"
    );

    return false;
  }

  // ===================================================
  // weather_code
  // ===================================================
  JsonArray codes =
    daily[
      "weather_code"
    ].as<JsonArray>();

  // ===================================================
  // max temperature
  // ===================================================
  JsonArray highs =
    daily[
      "temperature_2m_max"
    ].as<JsonArray>();

  // ===================================================
  // min temperature
  // ===================================================
  JsonArray lows =
    daily[
      "temperature_2m_min"
    ].as<JsonArray>();

  // ===================================================
  // Check essential weather data
  // ===================================================
  if (
    codes.isNull() ||
    highs.isNull() ||
    lows.isNull()
  ) {

    lastFetchError =
      "weather data missing";

    Serial.println(
      "ERROR: weather data missing"
    );

    return false;
  }

  // ===================================================
  // Rain probability
  //
  // 這個是非必要欄位
  // 沒有也不會讓程式失敗
  // ===================================================
  JsonArray rains =
    daily[
      "precipitation_probability_max"
    ].as<JsonArray>();

  if (
    rains.isNull()
  ) {

    Serial.println(
      "Warning: precipitation_probability_max missing"
    );
  }

  // ===================================================
  // 顯示天氣
  // ===================================================
  drawForecast(
    daily
  );

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "7-day weather parsed successfully"
  );

  Serial.println(
    "Weather displayed on TFT"
  );

  Serial.println(
    "================================"
  );

  return true;
}

// =====================================================
// WiFi 連線
// =====================================================
void connectWiFi() {

  WiFi.mode(
    WIFI_STA
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print(
    "WiFi connecting"
  );

  // 最多等待 20 秒
  for (
    uint8_t i = 0;
    i < 40 &&
    WiFi.status() != WL_CONNECTED;
    i++
  ) {

    delay(500);

    Serial.print(
      '.'
    );
  }

  Serial.println();

  // ===================================================
  // Connected
  // ===================================================
  if (
    WiFi.status() == WL_CONNECTED
  ) {

    Serial.print(
      "WiFi connected, IP: "
    );

    Serial.println(
      WiFi.localIP()
    );

  } else {

    Serial.println(
      "WiFi connection failed"
    );
  }
}

// =====================================================
// Setup
// =====================================================
void setup() {

  // ---------------------------------------------------
  // Serial
  // ---------------------------------------------------
  Serial.begin(
    115200
  );

  delay(500);

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "ESP32 KAOHSIUNG WEATHER"
  );

  Serial.println(
    "================================"
  );

  // ---------------------------------------------------
  // SPI
  // ---------------------------------------------------
  SPI.begin(
    TFT_CLK,
    -1,
    TFT_SDI,
    TFT_CS
  );

  // ---------------------------------------------------
  // TFT
  // ---------------------------------------------------
  tft.begin(
    SPI
  );

  tft.setOrientation(
    0
  );

  // ---------------------------------------------------
  // WiFi
  // ---------------------------------------------------
  connectWiFi();

  // ---------------------------------------------------
  // First weather update
  // ---------------------------------------------------
  if (
    !fetchWeather()
  ) {

    showError(
      lastFetchError.c_str()
    );
  }

  // ---------------------------------------------------
  // Timer
  // ---------------------------------------------------
  lastUpdate =
    millis();
}

// =====================================================
// Loop
// =====================================================
void loop() {

  // ===================================================
  // WiFi disconnected → reconnect
  // ===================================================
  if (
    WiFi.status() != WL_CONNECTED
  ) {

    Serial.println(
      "WiFi disconnected, reconnecting..."
    );

    connectWiFi();
  }

  // ===================================================
  // Update weather
  // ===================================================
  if (
    millis() - lastUpdate >=
    UPDATE_INTERVAL
  ) {

    Serial.println();
    Serial.println(
      "Updating weather..."
    );

    if (
      !fetchWeather()
    ) {

      showError(
        lastFetchError.c_str()
      );
    }

    lastUpdate =
      millis();
  }

  delay(100);
}


