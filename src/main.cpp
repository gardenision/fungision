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
  waterValve.write(0);
}

int idle = 0;
bool waterValveOpen = false;

void loop() {
  delay(2000);

  Serial.println("Idle : " + String(idle));

  // idle 2 minutes
//  if (idle >= 60) {
    float temperature = dht.readTemperature();

    if (isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    if (!waterValveOpen && temperature > 27) {
      waterValve.write(90);
      waterValveOpen = true;
      idle = 0;
      Serial.println("Valve opened");
    } else {
      waterValve.write(0);
      waterValveOpen = false;
      idle = 0;
      Serial.println("Valve closed");
    }
//  }
//  idle++;
}
