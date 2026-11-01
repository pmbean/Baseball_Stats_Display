#include <WiFi.h>
#include <ArduinoJson.h>
#include "Arduino_GigaDisplay_GFX.h"

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const int MAX_GAMES = 5;
const int HTTP_TIMEOUT_MS = 15000;

GigaDisplay_GFX tft;

struct GameScore {
  char awayAbbrev[8];
  char homeAbbrev[8];
  char status[20];
  int awayScore;
  int homeScore;
};

String getDateDaysAgo(int daysAgo) {
  time_t now = time(nullptr) - (daysAgo * 24L * 60L * 60L);
  struct tm timeInfo;
  gmtime_r(&now, &timeInfo);

  char buffer[11];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeInfo);
  return String(buffer);
}

bool connectWiFi() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    return false;
  }

  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

String buildScheduleRequest() {
  String url = "/api/v1/schedule?sportId=1";
  url += "&startDate=" + getDateDaysAgo(6);
  url += "&endDate=" + getDateDaysAgo(0);
  url += "&hydrate=team,linescore";
  return url;
}

bool fetchRecentGames(GameScore* games, int& count) {
  WiFiClient client;

  Serial.println("Requesting MLB schedule");
  if (!client.connect("statsapi.mlb.com", 80)) {
    Serial.println("Connection failed");
    return false;
  }

  String url = buildScheduleRequest();
  String request = "GET " + url + " HTTP/1.1\r\n";
  request += "Host: statsapi.mlb.com\r\n";
  request += "Connection: close\r\n\r\n";

  client.print(request);

  unsigned long start = millis();
  while (client.connected() == false && millis() - start < HTTP_TIMEOUT_MS) {
    delay(10);
  }

  String response = "";
  while (client.available() || client.connected()) {
    while (client.available()) {
      char c = client.read();
      response += c;
    }
    if (!client.connected() && millis() - start > HTTP_TIMEOUT_MS) {
      break;
    }
    delay(10);
  }

  client.stop();

  int headerEnd = response.indexOf("\r\n\r\n");
  if (headerEnd == -1) {
    Serial.println("Invalid HTTP response");
    return false;
  }

  String json = response.substring(headerEnd + 4);
  DynamicJsonDocument doc(32768);
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonArray dates = doc["dates"];
  count = 0;

  for (JsonVariant dateEntry : dates) {
    JsonArray games = dateEntry["games"];

    for (JsonVariant gameEntry : games) {
      if (count >= MAX_GAMES) {
        break;
      }

      const char* awayAbbrev = gameEntry["teams"]["away"]["team"]["abbreviation"];
      const char* homeAbbrev = gameEntry["teams"]["home"]["team"]["abbreviation"];
      const char* statusText = gameEntry["status"]["detailedState"];
      if (statusText == nullptr) {
        statusText = gameEntry["status"]["abstractGameState"];
      }
      if (statusText == nullptr) {
        statusText = "Unknown";
      }

      if (awayAbbrev == nullptr) {
        awayAbbrev = "AWAY";
      }
      if (homeAbbrev == nullptr) {
        homeAbbrev = "HOME";
      }

      GameScore& g = games[count];
      strncpy(g.awayAbbrev, awayAbbrev, sizeof(g.awayAbbrev) - 1);
      g.awayAbbrev[sizeof(g.awayAbbrev) - 1] = '\0';

      strncpy(g.homeAbbrev, homeAbbrev, sizeof(g.homeAbbrev) - 1);
      g.homeAbbrev[sizeof(g.homeAbbrev) - 1] = '\0';

      g.awayScore = gameEntry["teams"]["away"]["score"] | 0;
      g.homeScore = gameEntry["teams"]["home"]["score"] | 0;

      strncpy(g.status, statusText, sizeof(g.status) - 1);
      g.status[sizeof(g.status) - 1] = '\0';

      count++;
    }

    if (count >= MAX_GAMES) {
      break;
    }
  }

  return count > 0;
}

void drawHeader() {
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("MLB Scores");

  tft.drawFastHLine(0, 38, 480, GC9A01A_RED);
}

void drawGames(GameScore* games, int count) {
  drawHeader();

  for (int i = 0; i < count; i++) {
    int yPos = 52 + (i * 52);

    tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
    tft.setTextSize(2);
    tft.setCursor(12, yPos);
    tft.print(games[i].awayAbbrev);
    tft.print(" ");
    tft.print(games[i].awayScore);

    tft.setCursor(170, yPos);
    tft.print("@");

    tft.setCursor(215, yPos);
    tft.print(games[i].homeAbbrev);
    tft.print(" ");
    tft.print(games[i].homeScore);

    tft.setTextColor(GC9A01A_YELLOW, GC9A01A_BLACK);
    tft.setCursor(320, yPos);
    tft.print(games[i].status);

    tft.drawFastHLine(0, yPos + 22, 480, GC9A01A_DARKGREY);
  }
}

void showError(const char* message) {
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_RED, GC9A01A_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.println("MLB Scores");
  tft.setTextColor(GC9A01A_WHITE, GC9A01A_BLACK);
  tft.setCursor(10, 130);
  tft.println(message);
}

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(GC9A01A_BLACK);

  Serial.begin(115200);
  delay(1000);

  if (!connectWiFi()) {
    showError("WiFi failed");
    return;
  }

  GameScore games[MAX_GAMES];
  int gameCount = 0;

  if (!fetchRecentGames(games, gameCount)) {
    showError("API fetch failed");
    return;
  }

  drawGames(games, gameCount);
}

void loop() {
  delay(60000);
  GameScore freshGames[MAX_GAMES];
  int freshCount = 0;

  if (fetchRecentGames(freshGames, freshCount)) {
    drawGames(freshGames, freshCount);
  }
}
