#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

constexpr uint8_t SYMBOL_SIZE = 15;

const uint16_t N_SYMBOL[SYMBOL_SIZE] = {
  0b000001111100000,
  0b000111111111000,
  0b001111111111100,
  0b011100111001110,
  0b011100011001110,
  0b111100011001111,
  0b111100001001111,
  0b111100101001111,
  0b111100100001111,
  0b111100110001111,
  0b011100110001110,
  0b011100111001110,
  0b001111111111100,
  0b000111111111000,
  0b000001111100000
};

const uint16_t W_SYMBOL[SYMBOL_SIZE] = {
  0b000001111100000,
  0b000111111111000,
  0b001111111111100,
  0b011111111111110,
  0b010011101110010,
  0b110011000110011,
  0b110011000110011,
  0b111010010010111,
  0b111010010010111,
  0b111000111000111,
  0b011000111000110,
  0b011100111001110,
  0b001101111101100,
  0b000111111111000,
  0b000001111100000
};

const uint16_t M_SYMBOL[SYMBOL_SIZE] = {
  0b000001111100000,
  0b000111111111000,
  0b001111111111100,
  0b011001111100110,
  0b011000111000110,
  0b111000111000111,
  0b111000010000111,
  0b111000010000111,
  0b111001010100111,
  0b111001000100111,
  0b011001000100110,
  0b011001101100110,
  0b001111111111100,
  0b000111111111000,
  0b000001111100000
};

const uint16_t F_SYMBOL[SYMBOL_SIZE] = {
  0b000001111100000,
  0b000111111111000,
  0b001111111111100,
  0b011110000001110,
  0b011110000001110,
  0b111110011111111,
  0b111110011111111,
  0b111110000011111,
  0b111110011111111,
  0b111110011111111,
  0b011110011111110,
  0b011110011111110,
  0b001111111111100,
  0b000111111111000,
  0b000001111100000
};

enum SymbolID : uint8_t {
  SYMBOL_N,
  SYMBOL_W,
  SYMBOL_M,
  SYMBOL_F,
  SYMBOL_COUNT
};

SymbolID CHAR_TO_SYMBOL[128] = {};

struct Symbol {
  const uint16_t* bitmap;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

const Symbol SYMBOL_TABLE[SYMBOL_COUNT] = {
  {
    N_SYMBOL,
    50, 50, 0
  },
  {
    W_SYMBOL,
    50, 50, 0
  },
  {
    M_SYMBOL,
    50, 20, 4
  },
  {
    F_SYMBOL,
    50, 20, 4
  }
};

inline const Symbol& getSymbol(char c) {
  return SYMBOL_TABLE[CHAR_TO_SYMBOL[(uint8_t)c]];
}

MatrixPanel_I2S_DMA *dma_display = nullptr;

const char* BASE_URL = "<API_BASE_URL>";
const char* DEVICE_KEY = "<RANDOM_KEY>";
const char* WIFI_SSID = "<WIFI_SSID>";
const char* WIFI_PW = "<WIFI_PASSWORD>";
const char* stopId1 = "<STOP_ID_1>";
const char* stopId2 = "<STOP_ID_2>";

String fetchMTA(const char* urlSuffix, const char* stopId) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.begin(String(BASE_URL) + "?urlSuffix=" + String(urlSuffix) + "&stopId=" + String(stopId));
  http.setTimeout(15000);
  http.addHeader("x-device-key", DEVICE_KEY);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return "";
  }

  String payload = http.getString();

  http.end();

  return payload;
}

JsonDocument parseResponse(const String &response) {
  JsonDocument doc;
  deserializeJson(doc, response);
  return doc;
}

void drawSymbol(
  const uint16_t* bitmap,
  int16_t ox, int16_t oy,
  uint8_t r, uint8_t g, uint8_t b
) {
  for (uint8_t y = 0; y < SYMBOL_SIZE; y++) {
    uint16_t row = bitmap[y];

    for (uint8_t x = 0; x < SYMBOL_SIZE; x++) {
      if ((row & (1 << (SYMBOL_SIZE - 1 - x))) && (ox + x >= 0)) {
        dma_display->drawPixelRGB888(ox + x, oy + y , r, g, b);
      }
    }
  }
}

void drawSymbolWithText(
  const uint16_t* bitmap,
  int16_t ox, int16_t oy,
  uint8_t r, uint8_t g, uint8_t b,
  const char* text
) {
  drawSymbol(bitmap, ox, oy, r, g, b);
  dma_display->setCursor(ox + SYMBOL_SIZE + 5, oy + 4);
  dma_display->print(text);
}

void drawArrivalRows(JsonDocument& doc1, JsonDocument& doc2) {
  JsonArray arrivals1 = doc1["arrivals"].as<JsonArray>();
  JsonArray arrivals2 = doc2["arrivals"].as<JsonArray>();

  uint8_t size1 = arrivals1.size();
  uint8_t size2 = arrivals2.size();

  for (uint8_t i = 0; i < 6; i++) {
    dma_display->clearScreen();

    if (size1 == 0) {
      dma_display->setCursor(4, 4);
      dma_display->print("No data");
    } else {
      uint8_t idx1 = (size1 == 1) ? 0 : (i % 2);

      const char* line1 = arrivals1[idx1]["line"];
      int minutes1 = arrivals1[idx1]["minutes"];
      String arrivalText1 = String(minutes1) + " min";
      const Symbol& symbol1 = getSymbol(line1[0]);

      drawSymbolWithText(symbol1.bitmap, 0, 0, symbol1.r, symbol1.g, symbol1.b, arrivalText1.c_str());
    }

    if (size2 == 0) {
      dma_display->setCursor(4, SYMBOL_SIZE + 6);
      dma_display->print("No data");
    } else {
      uint8_t idx2 = (size2 == 1) ? 0 : (i % 2);

      const char* line2 = arrivals2[idx2]["line"];
      int minutes2 = arrivals2[idx2]["minutes"];
      String arrivalText2 = String(minutes2) + " min";
      const Symbol& symbol2 = getSymbol(line2[0]);

      drawSymbolWithText(symbol2.bitmap, 0, SYMBOL_SIZE + 2, symbol2.r, symbol2.g, symbol2.b, arrivalText2.c_str());
    }

    delay(5000);
  }
}

void setup() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  mxconfig.clkphase = true;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(40);
  dma_display->setTextColor(dma_display->color444(50, 50, 50));
  dma_display->clearScreen();

  delay(500);

  dma_display->setCursor(0, 0);
  dma_display->print("Hello");
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PW);
  while(WiFi.status() != WL_CONNECTED){
    dma_display->clearScreen();
    dma_display->setCursor(0, 0);
    dma_display->print("Connecting..");
    delay(1000);
  }

  CHAR_TO_SYMBOL['N'] = SYMBOL_N;
  CHAR_TO_SYMBOL['W'] = SYMBOL_W;
  CHAR_TO_SYMBOL['M'] = SYMBOL_M;
  CHAR_TO_SYMBOL['F'] = SYMBOL_F;

  dma_display->clearScreen();
}

void loop() {
  String nqrwResponse = fetchMTA("nqrw", stopId1);
  String bdfmResponse = fetchMTA("bdfm", stopId2);

  JsonDocument nqrwJson = parseResponse(nqrwResponse);
  JsonDocument bdfmJson = parseResponse(bdfmResponse);

  dma_display->clearScreen();
  drawArrivalRows(nqrwJson, bdfmJson);
}
