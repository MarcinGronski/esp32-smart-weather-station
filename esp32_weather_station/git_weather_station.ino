#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <ArduinoOTA.h>
#include "time.h"

// =====================================================
// Wi-Fi & API settings
// =====================================================
const char* ssid = "";
const char* password = "";
const char* apiKey = "";
const char* city = "";
const char* ntpServerName = "pool.ntp.org";



// =====================================================
// NodeMCU temperature server
// =====================================================
const char* server_ip = "";
const uint16_t server_port = 12345;

// =====================================================
// TFT
// =====================================================
TFT_eSPI tft = TFT_eSPI();

// =====================================================
// Time / Date
// =====================================================
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

String dateStr = "";
String dayStr = "";

// =====================================================
// Weather data
// =====================================================
// Temperatura z NodeMCU jest przechowywana osobno od danych
// OpenWeather, dzięki czemu pobieranie pogody nie nadpisuje
// temperatury z lokalnego serwera.
float nodeTemperature = NAN;
float weatherApiTemperature = NAN;
int weatherHumidity = -1;
float weatherWindSpeed = NAN;
String weatherDescription = "";

// =====================================================
// Screen memory
// =====================================================
String lastTimeStr = "";
String lastDateStr = "";
float lastTemp = NAN;
int lastHumidity = -1;
float lastWindSpeed = NAN;
String lastWeatherDescription = "";

// =====================================================
// Colors
// =====================================================
uint16_t customWatch = 0xF81F;
uint16_t customText = 0x0DA2;

// =====================================================
// Brightness
// =====================================================
#define BL_CHANNEL 0
#define DAY_BRIGHTNESS 255
#define NIGHT_BRIGHTNESS 40
int currentBrightness = -1;
// =====================================================
// Timers
// =====================================================
unsigned long previousMillis = 0;
const unsigned long DISPLAY_INTERVAL = 250;       // płynna pętla UI
const unsigned long WEATHER_INTERVAL = 600000;    // 10 minut
const unsigned long TEMP_INTERVAL = 300000;         // 5 minut
unsigned long lastDisplayUpdate = 0;

// =====================================================
// Network task synchronization
// =====================================================
SemaphoreHandle_t dataMutex = nullptr;

// Dane pobierane przez osobne zadanie FreeRTOS.
// Dzięki temu HTTP/WiFi nie blokuje pętli odpowiedzialnej
// za TFT, zegar i jasność.
volatile bool weatherFetchRequested = false;
volatile bool tempFetchRequested = false;
volatile bool networkTaskRunning = false;
volatile bool weatherDataValid = false;
volatile bool tempDataValid = false;

unsigned long lastWeatherFetchRequest = 0;
unsigned long lastTempFetchRequest = 0;

// =====================================================
// Funkcje pomocnicze - bezpieczny dostęp do danych
// =====================================================
void setNodeTemperature(float value) {
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    nodeTemperature = value;
    tempDataValid = true;
    xSemaphoreGive(dataMutex);
  }
}

void setWeatherData(float apiTemp, int humidity, float windSpeed, const String& description) {
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    weatherApiTemperature = apiTemp;
    weatherHumidity = humidity;
    weatherWindSpeed = windSpeed;
    weatherDescription = description;
    weatherDataValid = true;
    xSemaphoreGive(dataMutex);
  }
}

void copyWeatherData(float &temp, int &humidity, float &windSpeed, String &description,
                     bool &hasTemp, bool &hasWeather) {
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    temp = nodeTemperature;
    humidity = weatherHumidity;
    windSpeed = weatherWindSpeed;
    description = weatherDescription;
    hasTemp = tempDataValid;
    hasWeather = weatherDataValid;
    xSemaphoreGive(dataMutex);
  }
}

// =====================================================
// Pobieranie temperatury z NodeMCU
// Ta funkcja może chwilowo czekać, ale działa w osobnym
// zadaniu FreeRTOS, więc NIE blokuje TFT.
// =====================================================
void fetchTemperatureFromServer() {
  WiFiClient client;

  client.setTimeout(1000);

  if (!client.connect(server_ip, server_port)) {
    Serial.println("[TEMP] Brak polaczenia z NodeMCU");
    client.stop();
    return;
  }

  Serial.println("[TEMP] Wysylam: TEMP");

  // NodeMCU oczekuje dokładnie komendy TEMP
  client.println("TEMP");

  String response;
  response.reserve(32);

  unsigned long startTime = millis();

  while (millis() - startTime < 1500) {
    while (client.available()) {
      char c = client.read();
      response += c;
    }

    if (response.length() > 0) {
      break;
    }

    delay(1);
  }

  client.stop();

  response.trim();

  Serial.print("[TEMP] Odpowiedz NodeMCU: ");
  Serial.println(response);

  if (response.length() == 0) {
    Serial.println("[TEMP] Brak odpowiedzi");
    return;
  }

  float temperature = response.toFloat();

  if (temperature > -100.0 && temperature < 125.0) {
    Serial.print("[TEMP] Temperatura: ");
    Serial.print(temperature, 2);
    Serial.println(" C");

    setNodeTemperature(temperature);
  } else {
    Serial.println("[TEMP] Nieprawidlowa temperatura");
  }
}

// =====================================================
// OpenWeather
// =====================================================
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WEATHER] WiFi niedostepne");
    return;
  }

  String url =
      "http://api.openweathermap.org/data/2.5/weather?q=" +
      String(city) +
      "&units=metric&appid=" +
      String(apiKey);

  HTTPClient http;

  // Krótkie timeouty - działają w osobnym tasku,
  // więc nie zatrzymują wyświetlania.
  http.setConnectTimeout(800);
  http.setTimeout(1500);

  if (!http.begin(url)) {
    Serial.println("[WEATHER] Nie mozna otworzyc polaczenia");
    return;
  }

  int code = http.GET();

  if (code == HTTP_CODE_OK) {
    String payload = http.getString();

    // Większy bufor niż wcześniej - pozwala bezpiecznie
    // odczytać także opis pogody.
    DynamicJsonDocument doc(4096);

    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Odczyt dokładnie tak jak w oryginalnym kodzie,
      // ale z kontrolą, czy pola rzeczywiście istnieją.
      float apiTemp = doc["main"]["temp"].as<float>();
      int humidity = doc["main"]["humidity"].as<int>();
      float windSpeed = doc["wind"]["speed"].as<float>();
      String description = doc["weather"][0]["description"].as<String>();

      Serial.printf(
        "[WEATHER RAW] humidity=%d wind=%.2f desc=%s\\n",
        humidity,
        windSpeed,
        description.c_str()
      );

      // Nie zapisujemy pustych/błędnych danych jako nowych danych.
      if (doc["main"]["humidity"].isNull() ||
          doc["wind"]["speed"].isNull()) {
        Serial.println("[WEATHER] Brak main.humidity lub wind.speed w JSON");
      } else {
        setWeatherData(apiTemp, humidity, windSpeed, description);
      }

      Serial.printf(
        "[WEATHER] temp=%.1f humidity=%d wind=%.1f desc=%s\n",
        apiTemp,
        humidity,
        windSpeed,
        description.c_str()
      );
    } else {
      Serial.printf("[WEATHER] JSON error: %s\n", error.c_str());
    }
  } else {
    Serial.printf("[WEATHER] HTTP error: %d\n", code);
  }

  http.end();
}

// =====================================================
// Network FreeRTOS Task
// =====================================================
void networkTask(void* parameter) {
  networkTaskRunning = true;

  bool wifiStarted = false;
  bool wasConnected = false;
  unsigned long lastWifiRetry = 0;

  for (;;) {
    // -----------------------------------------------
    // WiFi - bez blokującego while() w setup()
    // -----------------------------------------------
    if (WiFi.status() != WL_CONNECTED) {
      wasConnected = false;

      if (!wifiStarted || millis() - lastWifiRetry >= 10000) {
        Serial.println("[WIFI] Proba polaczenia...");
        WiFi.begin(ssid, password);
        wifiStarted = true;
        lastWifiRetry = millis();
      }

      // Nie wykonujemy dalszych operacji sieciowych,
      // dopóki WiFi nie jest gotowe.
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    // -----------------------------------------------
    // Po pierwszym poprawnym połączeniu z WiFi
    // od razu pobieramy oba zestawy danych.
    // -----------------------------------------------
    if (!wasConnected) {
      wasConnected = true;
      weatherFetchRequested = true;
      tempFetchRequested = true;
      Serial.println("[WIFI] Polaczono - pobieram dane startowe");
    }

    // -----------------------------------------------
    // OpenWeather
    // -----------------------------------------------
    if (weatherFetchRequested) {
      weatherFetchRequested = false;
      fetchWeather();
    }

    // -----------------------------------------------
    // NodeMCU temperatura
    // -----------------------------------------------
    if (tempFetchRequested) {
      tempFetchRequested = false;
      fetchTemperatureFromServer();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// =====================================================
// Request network operations without blocking
// =====================================================
void requestWeatherFetch() {
  if (WiFi.status() == WL_CONNECTED && !weatherFetchRequested) {
    weatherFetchRequested = true;
  }
}

void requestTemperatureFetch() {
  if (WiFi.status() == WL_CONNECTED && !tempFetchRequested) {
    tempFetchRequested = true;
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  // PWM
  ledcSetup(BL_CHANNEL, 5000, 8);
  ledcAttachPin(TFT_BL, BL_CHANNEL);

  // TFT
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  tft.setCursor(10, 10);
  tft.println("Uruchamianie...");

  // Jasność
  setBrightness();

  // ---------------------------------------------------
  // WiFi uruchamiamy, ale NIE czekamy tutaj w pętli.
  // ---------------------------------------------------
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(ssid, password);

  // NTP
  configTime(7200, 0, ntpServerName);
  setupOTA();

  // Mutex
  dataMutex = xSemaphoreCreateMutex();

  // ---------------------------------------------------
  // Zadanie sieciowe na drugim rdzeniu.
  // Pętla loop() pozostaje odpowiedzialna za TFT.
  // ---------------------------------------------------
  xTaskCreatePinnedToCore(
    networkTask,
    "NetworkTask",
    8192,
    nullptr,
    1,
    nullptr,
    0
  );

  // Pierwsze pobranie zostanie automatycznie zlecone
  // w chwili wykrycia poprawnego połączenia WiFi.

  tft.fillScreen(TFT_BLACK);

  Serial.println("[SYSTEM] Start - UI nie jest blokowane przez siec");
}


void setupOTA() {
  ArduinoOTA.setHostname("ESP32-WeatherStation");
  ArduinoOTA.setPassword("");
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  ArduinoOTA.handle();
  unsigned long now = millis();

  // Pętla UI działa niezależnie od sieci.
  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;

    getTime();
    getDate();
    setBrightness();
    displayScreen();
  }

  // ---------------------------------------------------
  // Zlecanie pobierania pogody.
  // Nie wykonujemy fetchWeather() bezpośrednio tutaj!
  // ---------------------------------------------------
  if (WiFi.status() == WL_CONNECTED &&
      now - lastWeatherFetchRequest >= WEATHER_INTERVAL) {

    lastWeatherFetchRequest = now;
    requestWeatherFetch();
  }

  // ---------------------------------------------------
  // Zlecanie odczytu temperatury NodeMCU.
  // Nie wykonujemy fetchTemperatureFromServer()
  // bezpośrednio tutaj!
  // ---------------------------------------------------
  if (WiFi.status() == WL_CONNECTED &&
      now - lastTempFetchRequest >= TEMP_INTERVAL) {

    lastTempFetchRequest = now;
    requestTemperatureFetch();
  }

  // loop() nie wykonuje delay() i nie czeka na sieć.
  // Oddajemy minimalnie czas FreeRTOS.
  delay(1);
}

// =====================================================
// TIME
// =====================================================
void getTime() {
  struct tm timeinfo;

  // Timeout 0 ms - funkcja nie czeka na synchronizację NTP.
  if (getLocalTime(&timeinfo, 0)) {
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
  }
}

// =====================================================
// DATE and Week Day
// =====================================================
void getDate() {
  struct tm timeinfo;

  // Timeout 0 ms - brak blokowania ekranu.
  if (getLocalTime(&timeinfo, 0)) {
    char buffer[20];

    snprintf(
      buffer,
      sizeof(buffer),
      "%02d.%02d.%04d",
      timeinfo.tm_mday,
      timeinfo.tm_mon + 1,
      timeinfo.tm_year + 1900
    );

    dateStr = String(buffer);

    const char* days[] = {
      "Niedziela",
      "Poniedzialek",
      "Wtorek",
      "Sroda",
      "Czwartek",
      "Piatek",
      "Sobota"
    };

    dayStr = days[timeinfo.tm_wday];
  }
}

// =====================================================
// Auto brightness
// =====================================================
void setBrightness() {
  struct tm timeinfo;

  // Timeout 0 ms - nie blokujemy UI.
  if (getLocalTime(&timeinfo, 0)) {
    int h = timeinfo.tm_hour;

    int value =
      (h >= 22 || h < 7)
      ? NIGHT_BRIGHTNESS
      : DAY_BRIGHTNESS;

    if (value != currentBrightness) {
      ledcWrite(BL_CHANNEL, value);
      currentBrightness = value;
    }
  }
}

// =====================================================
// Weather icon
// =====================================================
void drawWeatherIcon(int x, int y) {
  String description;

  float temp;
  int humidity;
  float windSpeed;
  bool hasTemp = false;
  bool hasWeather = false;
  uint16_t SUN_YELLOW = tft.color565(255, 220, 0);
  copyWeatherData(temp, humidity, windSpeed, description, hasTemp, hasWeather);

  // Czyścimy tylko obszar ikony i robimy to wyłącznie wtedy,
  // gdy opis pogody faktycznie się zmienił.
  tft.fillRect(x - 35, y - 35, 70, 70, TFT_BLACK);

  if (!hasWeather) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("...", x, y);
    return;
  }

  if (description.indexOf("clear") >= 0) {
    tft.fillCircle(x, y, 15, TFT_YELLOW);

    for (int i = 0; i < 8; i++) {
      float a = i * PI / 4;
      int x1 = x + cos(a) * 22;
      int y1 = y + sin(a) * 22;
      int x2 = x + cos(a) * 30;
      int y2 = y + sin(a) * 30;
      tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
    }
  } else if (description.indexOf("cloud") >= 0) {
    tft.fillCircle(x - 10, y, 12, TFT_WHITE);
    tft.fillCircle(x + 8, y - 5, 15, TFT_WHITE);
    tft.fillRect(x - 25, y, 50, 15, TFT_WHITE);
  } else if (description.indexOf("rain") >= 0 ||
             description.indexOf("drizzle") >= 0) {
    tft.fillCircle(x, y - 5, 18, TFT_DARKGREY);

    for (int i = -1; i <= 1; i++) {
      tft.drawLine(
        x + i * 10,
        y + 15,
        x + i * 10 - 5,
        y + 28,
        TFT_BLUE
      );
    }
  } else {
    tft.fillCircle(x, y, 15, TFT_CYAN);
  }
}

// =====================================================
// DISPLAY
// =====================================================
void displayScreen() {
  int w = tft.width();

  float temp;
  int humidity;
  float windSpeed;
  String description;
  bool hasTemp = false;
  bool hasWeather = false;

  copyWeatherData(
    temp,
    humidity,
    windSpeed,
    description,
    hasTemp,
    hasWeather
  );

  // -----------------------------------------------
  // Godzina - aktualizowana tylko po zmianie sekundy
  // -----------------------------------------------
  char timeBuffer[9];

  snprintf(
    timeBuffer,
    sizeof(timeBuffer),
    "%02d:%02d:%02d",
    currentHour,
    currentMinute,
    currentSecond
  );

  String timeStr = String(timeBuffer);

  if (timeStr != lastTimeStr) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(4);
    tft.setTextColor(customWatch, TFT_BLACK);
    tft.drawString(timeStr, w / 2, 27);

    lastTimeStr = timeStr;
  }

  // -----------------------------------------------
  // Data - tylko po zmianie
  // -----------------------------------------------
  String fullDate = dateStr + " " + dayStr;

  if (fullDate != lastDateStr) {
    tft.fillRect(0, 60, w, 35, TFT_BLACK);

    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(fullDate, w / 2, 75);

    lastDateStr = fullDate;
  }

  // -----------------------------------------------
  // Ikona - tylko po zmianie opisu pogody
  // -----------------------------------------------
  if (description != lastWeatherDescription) {
    drawWeatherIcon(45, 145);
    lastWeatherDescription = description;
  }

  // Pierwsze narysowanie ikony, jeśli jeszcze nie ma opisu.
  if (lastWeatherDescription == "" && !hasWeather) {
    drawWeatherIcon(45, 145);
  }

  // -----------------------------------------------
  // Temperatura - aktualizujemy tylko jej własny obszar
  // Bez fillRect(0,100,320,180), więc nic nie mruga.
  // -----------------------------------------------
  if (hasTemp && (isnan(lastTemp) || fabs(temp - lastTemp) > 0.05)) {
    tft.fillRect(185, 140, 120, 25, TFT_BLACK);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.drawFloat(temp, 1, 190, 145);
    tft.drawString(" C", 240, 145);

    lastTemp = temp;
  }

  // -----------------------------------------------
  // Wilgotność - tylko po zmianie
  // -----------------------------------------------
  if (hasWeather && humidity != lastHumidity) {
    tft.fillRect(185, 165, 120, 25, TFT_BLACK);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.drawNumber(humidity, 190, 170);
    tft.drawString("%", 220, 170);

    lastHumidity = humidity;
  }

  // -----------------------------------------------
  // Wiatr - tylko po zmianie
  // -----------------------------------------------
  if (hasWeather &&
      (isnan(lastWindSpeed) || fabs(windSpeed - lastWindSpeed) > 0.05)) {

    tft.fillRect(185, 190, 120, 25, TFT_BLACK);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.drawFloat(windSpeed, 1, 190, 195);
    tft.drawString(" m/s", 240, 195);

    lastWindSpeed = windSpeed;
  }

  // -----------------------------------------------
  // Stałe etykiety rysujemy tylko raz.
  // -----------------------------------------------
  static bool labelsDrawn = false;

  if (!labelsDrawn) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);

    tft.setTextColor(customText, TFT_BLACK);
    tft.drawString("Pogoda:", 100, 115);
    tft.drawString("Temp:   ", 100, 145);
    tft.drawString("Wilg:   ", 100, 170);
    tft.drawString("Wiatr:  ", 100, 195);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(city, 190, 115);

    labelsDrawn = true;
  }

  // -----------------------------------------------
  // Jeśli dane jeszcze nie przyszły, pokazujemy N/A
  // tylko raz, a nie przy każdym odświeżeniu.
  // -----------------------------------------------
  if (!hasTemp && isnan(lastTemp)) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("N/A", 190, 145);
  }

  if (!hasWeather && lastHumidity == -1) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("N/A", 190, 170);
  }

  if (!hasWeather && isnan(lastWindSpeed)) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("N/A", 190, 195);
  }
}