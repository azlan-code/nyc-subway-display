# NYC Subway Display (ESP32 + HUB75 LED Matrix)

A tiny “next train” sign for the NYC subway.

This repo contains the firmware that runs on an **ESP32** connected to a **64×32 HUB75 LED matrix panel**. It shows arrival times for the next trains by calling a companion API (see the **nyc-subway-proxy** repo) that converts the MTA’s real-time feed into simple JSON that’s microcontroller-friendly.

> This project is not affiliated with the MTA. Data comes from MTA GTFS Realtime.

---

## What it does

- Connects the ESP32 to Wi‑Fi
- Calls a JSON API endpoint (your deployed proxy) for real-time arrivals
- Parses the JSON response
- Displays:
  - A colored “line bullet” icon (custom 15×15 bitmap)
  - The arrival time (e.g., `5 min`)
- Shows two stops/feeds in two rows (top + bottom)
- Cycles between the next arrivals every 5 seconds

### Why there’s a proxy

The MTA GTFS Realtime feed is **Protocol Buffers** (protobuf), not JSON. Decoding protobuf on an ESP32 is doable but annoying. The proxy turns it into clean JSON so the ESP32 can stay simple and reliable.

---

## How the repos fit together

**MTA GTFS Realtime (protobuf)** → **[nyc-subway-proxy (AWS)](https://github.com/azlan-code/nyc-subway-proxy)** → **this ESP32 display (JSON)**

The ESP32 calls the proxy like:

`GET <API_BASE_URL>?urlSuffix=nqrw&stopId=<STOP_ID>`

…and receives JSON like:

```json
{
  "station": "R20N",
  "arrivals": [
    { "line": "N", "minutes": 5 },
    { "line": "N", "minutes": 9 }
  ]
}
```

## Software prerequisites

- Arduino IDE **or** PlatformIO
- ESP32 board support installed
- Libraries:
  - [ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/tidbyt/ESP32-HUB75-MatrixPanel-I2S-DMA.git)
  - WiFi (ESP32 core)
  - HTTPClient (ESP32 core)
  - ArduinoJson (Arduino Library Manager)

## Configure

Edit these placeholders in the sketch:

```cpp
const char* BASE_URL = "<API_BASE_URL>";
const char* WIFI_SSID = "<WIFI_SSID>";
const char* WIFI_PW = "<WIFI_PASSWORD>";
const char* stopId1 = "<STOP_ID_1>";
const char* stopId2 = "<STOP_ID_2>";
```

### Choosing feeds / lines

This firmware currently fetches two feeds:

```cpp
String nqrwResponse = fetchMTA("nqrw", stopId1);
String bdfmResponse = fetchMTA("bdfm", stopId2);
```

Common feed groups:
- `nqrw` typically includes N/Q/R/W
- `bdfm` typically includes B/D/F/M

Adjust `urlSuffix` values to match your station’s lines.

### Stop IDs

Stop IDs come from the MTA’s static GTFS stop list and often include direction (northbound/southbound), e.g. `R20N`.

If you see “No data”, the stop ID is the first thing to verify.

## Build + upload

1. Open the project in Arduino IDE (or PlatformIO)
2. Select your ESP32 board + serial port
3. Upload
4. The matrix will:
   - print `Hello`
   - show `Connecting..` until Wi‑Fi connects
   - then start cycling arrivals

## Refresh timing

The display swaps what it shows every 5 seconds, but the code fetches new API data once per loop and then cycles 6 times:

- 6 screens × 5 seconds = ~30 seconds between API refreshes

To change this, adjust:
- `delay(5000);`
- the `for (uint8_t i = 0; i < 6; i++) { ... }` loop
- or move the API fetch inside the loop

## Customization

### Add more line icons

Icons currently exist for:
- N, W (yellow)
- M, F (orange)

To add more:
1. Add a 15×15 bitmap array
2. Extend `SymbolID` and `SYMBOL_TABLE`
3. Map the route letter:
   ```cpp
   CHAR_TO_SYMBOL['A'] = SYMBOL_A;
   ```

### Brightness

```cpp
dma_display->setBrightness8(40);
```
