# Baseball_Stats_Display

A simple Arduino GIGA app that connects to Wi‑Fi, pulls recent MLB game data from the MLB Stats API, and displays game scores on the built‑in display.

## What this app does
- Connects to your Wi‑Fi network
- Fetches MLB games from the last 7 days from `statsapi.mlb.com`
- Parses the schedule JSON
- Displays the most recent games on the GIGA display
- Shows team abbreviations and scores with game status

## Hardware
- Arduino GIGA R1 WiFi
- Built‑in GIGA display
- Wi‑Fi access point with internet connectivity

## Required Arduino libraries
In Arduino IDE, install:
- `Arduino_GigaDisplay_GFX`
- `ArduinoJson`
- `WiFiS3`

## Configuration
Edit the top of the sketch and set your Wi‑Fi credentials:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

## Sketch location
- `MLB_Scoreboard_Giga/MLB_Scoreboard_Giga.ino`

## Notes
- This uses the MLB Stats API endpoint: `https://statsapi.mlb.com/api/v1/schedule`
- The app requests games from the last 7 days and displays up to 5 recent games
- It uses a simple `setInsecure()` TLS connection for quick setup; if you want stricter certificate verification, replace it with a proper cert workflow

## Example behavior
The display shows rows like:
- BOS 4 vs NYY 2 — Final
- LAD 7 vs SF 6 — Final
- MIA 3 vs ATL 3 — Live 6th
