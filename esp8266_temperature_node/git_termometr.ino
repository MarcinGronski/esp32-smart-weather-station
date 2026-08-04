#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>

// Konfiguracja WiFi
const char* ssid = "";
const char* password = "";

// Ustawienia statycznego IP
IPAddress local_IP(192, 168, 0, 105);
IPAddress gateway(192, 168, 0, 1);       // Dostosuj do swojej sieci
IPAddress subnet(255, 255, 255, 0);

#define ONE_WIRE_BUS D2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiServer server(12345);
float currentTemperature = -127.0;

void setup() {
  Serial.begin(115200);
  sensors.begin();

  // Konfiguracja statycznego IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Błąd konfiguracji statycznego IP");
  }

  // Połączenie WiFi
  WiFi.begin(ssid, password);
  Serial.print("Laczenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Polaczono!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Uruchomienie serwera TCP
  server.begin();

  // Usunięto mDNS
}

void loop() {
  // Odświeżanie temperatury
  sensors.requestTemperatures();
  currentTemperature = sensors.getTempCByIndex(0);

  // Obsługa klientów TCP
  WiFiClient client = server.accept();
  if (client) {
    Serial.println("Klient się połączył");
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') break;
        request += c;
      }
    }
    Serial.print("Odebrane zapytanie: ");
    Serial.println(request);

    // Obsługa prostych zapytań HTTP
    if (request.indexOf("GET / ") >= 0 || request.indexOf("GET / HTTP") >= 0) {
      String response = "<html><head><title>Temperatura</title></head><body>";
      response += "<h1>Aktualna temperatura: ";
      response += String(currentTemperature, 2);
      response += " &deg;C</h1></body></html>";
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      client.print(response);
    } else {
      // Zwrot samej wartości temperatury
      String response = String(currentTemperature, 2) + " &deg;C\n";
      client.print(response);
    }
    client.stop();
    Serial.println("Klient rozłączony");
  }

  delay(100);
}