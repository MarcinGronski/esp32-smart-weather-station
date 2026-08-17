#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* ssid = "";
const char* password = "";

const char* webhook =
"https://discordapp.com/api/webhooks/";

void setup() {

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Połączono z WiFi");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  if (https.begin(client, webhook)) {

    https.addHeader("Content-Type", "application/json");

    String json =
    "{\"content\":\"Wiadomość z ESP32!\"}";

    int code = https.POST(json);

    Serial.print("HTTP Code: ");
    Serial.println(code);

    String response = https.getString();

    Serial.println(response);

    https.end();
  }
}

void loop() {

}