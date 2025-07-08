#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Setting WiFi
const char* ssid = "Pandan D";
const char* password = "takonibu354";

// API Endpoint
const char* apiUrl = "https://api.gardenision.com/api";

const char* serial_number = "skripsiazizah";

const char* bearer_token = "67|PsWZsBerxoSiCTgIBBZVCpC7BzMIdfaNF1d9nuQn63c839e0";

// Pin setup
#define DHTPIN D1          // DHT22 di D1 (GPIO5)
#define DHTTYPE DHT22      // Tipe sensor
#define RELAYPIN D0        // Relay di D0 (GPIO16)

DHT dht(DHTPIN, DHTTYPE);

// Data sebelumnya
float lastTemp = 0.0;
float lastHumidity = 0.0;

bool pumpOn = 0;

const unsigned long tempInterval = 10UL * 60UL * 1000UL; // 10 menit
unsigned long lastTempTime = 0;

void sendData(int id, const char* value) {
  // add enter
  Serial.println();

  if (value == NULL || strlen(value) == 0) {
    Serial.println("Value is null or empty!");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = String(apiUrl) + "/device/" + String(serial_number) + "/module/" + String(id) + "/logs";

    if (!http.begin(client, url)) {
      Serial.println("HTTP begin failed!");
      return;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Requested-With", "XMLHttpRequest");
    http.addHeader("Authorization", "Bearer " + String(bearer_token));

    JsonDocument doc;
    doc["level"] = "info";

    JsonObject context = doc["context"].to<JsonObject>();
    context["value"] = value;

    String requestBody;
    serializeJson(doc, requestBody);

    Serial.println("URL: " + url);
    Serial.println("Body: " + requestBody);
    Serial.println("ID: " + String(id));
    Serial.println("Value: " + String(value));

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      Serial.print("Data sent, response code: ");
      Serial.println(httpResponseCode);
      Serial.println("Response: " + http.getString());
    } else {
      Serial.print("Error sending data, code: ");
      Serial.println(httpResponseCode);
      Serial.println("Response: " + http.getString());
    }

    doc.clear();
    http.end();

    Serial.println();

  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  dht.begin();
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH); // Matikan water pump di awal

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");


}

void loop() {
  // Data sensor
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  JsonObject temperature = array.add<JsonObject>();
  temperature["id"] = 7;
  temperature["value"] = dht.readTemperature();
  // temperature["value"] = isnan(dht.readTemperature()) ? (lastTemp ? (float)lastTemp + 1.0 : 27.0) : dht.readTemperature();

  JsonObject humidity = array.add<JsonObject>();
  humidity["id"] = 6;
  humidity["value"] = dht.readHumidity();
  // humidity["value"] = isnan(dht.readHumidity()) ? (lastHumidity ? (float)lastHumidity + 1.0 : 50.0) : dht.readHumidity();

  JsonObject pump = array.add<JsonObject>();
  pump["id"] = 5;
  pump["value"] = pumpOn ? 1 : 0;

  Serial.println();
  Serial.println("Pump: " + String(pump["value"]));
  Serial.println();

  // Cek timer untuk water pump
  unsigned long now = millis();

  String modules;
  serializeJson(doc, modules);

  if (isnan(temperature["value"]) || isnan(humidity["value"]) || temperature["value"] == NULL || humidity["value"] == NULL) {
    Serial.println("Failed to read from DHT sensor!");
    // delay(600000);
    delay(1000);
    return;
  } else {
    Serial.print("Temperature: ");
    Serial.print(String(temperature["value"]));
    Serial.print(" C, Humidity: ");
    Serial.println(String(humidity["value"]));
  }
  

  // Kirim ke API kalau ada perubahan data every 10 menit
  if (lastTempTime == 0 || (now - lastTempTime >= tempInterval)) {
    lastTempTime = now;
    
    for (size_t i = 0; i < doc.size(); i++) {
      JsonObject module = doc[i];
      if (module["id"] == 1) {
        if (temperature["value"] != lastTemp) {
          Serial.println("Sending temperature data : " + String(temperature["value"]) + " => " + String(lastTemp));
          sendData(module["id"], String(temperature["value"]).c_str());
          lastTemp = temperature["value"];
        }
      } else if (module["id"] == 2) {
        if (humidity["value"] != lastHumidity) {
          Serial.println("Sending humidity data : " + String(humidity["value"]) + " => " + String(lastHumidity));
          sendData(module["id"], String(humidity["value"]).c_str());
          lastHumidity = humidity["value"];
        }
      }
    }
  }

  Serial.println();
  Serial.println("// DEBUG //");
  Serial.println("PumpOn: " + String(pumpOn));
  Serial.println("now: " + String(now));
  Serial.println("lastTemp: " + String(lastTemp));
  Serial.println("lastHumidity: " + String(lastHumidity));
  Serial.println();

  if (!pumpOn && (lastTemp > 28.0 || lastHumidity < 80)) {
    digitalWrite(RELAYPIN, LOW); // Nyalakan water pump
    pump["value"] = 1;
    Serial.println("Water pump ON!");
  } else if (pumpOn && (lastTemp < 28.0 || lastHumidity > 80)) {
    digitalWrite(RELAYPIN, HIGH); // Matikan water pump
    pump["value"] = 0;
    Serial.println("Water pump OFF!");
  }

  if (pump["value"] != pumpOn) {
    Serial.println("Sending pump data : " + String(pump["value"]) + " => " + String(pumpOn));
    sendData(pump["id"], String(pump["value"]).c_str());
    pumpOn = pump["value"];
  }
  doc.clear();

  // delay(600000);
  delay(1000);
}
