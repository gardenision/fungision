#include <Arduino.h>
#include <DHT.h>
#include <Servo.h>

#define DHTPIN D4
#define SERVO_PIN D2

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Servo waterValve;

void setup() {
  Serial.begin(115200);

  dht.begin();

  waterValve.attach(SERVO_PIN);
}

int idle = 0;
bool waterValveOpen = false;

void loop() {
  delay(2000);

  if (waterValveOpen) {
    waterValve.write(0);
    waterValveOpen = false;
  }

  // idle 5 minutes
  if (idle >= 150) {
    float temperature = dht.readTemperature();

    if (isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    if (temperature > 27) {
      waterValve.write(90);
      waterValveOpen = true;
      idle = 0;
      Serial.println("Valve opened");
    } else {
      waterValve.write(0);
      Serial.println("Valve closed");
    }
  }
  idle++;
}
