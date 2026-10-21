#include <ModbusMaster.h>

#define RXD2 4
#define TXD2 15
#define DIR_PIN 2

ModbusMaster node;

void preTransmission() {
  digitalWrite(DIR_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(DIR_PIN, LOW);
}

unsigned long lastReadTime = 0;
const long readInterval = 3000;

void setup() {
  Serial.begin(115200);

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  node.begin(20, Serial2);

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.println("ESP32 Modbus Master Initialized with DIR Control on IO2.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;

    // 1. Read temperature (0x0000, 2 registers)
    uint8_t resultTemp = node.readInputRegisters(0x0000, 2);

    if (resultTemp == node.ku8MBSuccess) {
      uint32_t rawTemp =
          ((uint32_t)node.getResponseBuffer(0) * 65536UL)
          + node.getResponseBuffer(1);

      float temp = (int32_t)rawTemp / 1000.0;

      Serial.print("Temperature: ");
      Serial.print(temp, 2);
      Serial.println(" °C");
    } else {
      Serial.print("Read Temp Failed! Error: 0x");
      Serial.println(resultTemp, HEX);
    }

    delay(100);

    // 2. Read humidity (0x0002, 2 registers)
    uint8_t resultHum = node.readInputRegisters(0x0002, 2);

    if (resultHum == node.ku8MBSuccess) {
      uint32_t rawHum =
          ((uint32_t)node.getResponseBuffer(0) * 65536UL)
          + node.getResponseBuffer(1);

      float humidity = (int32_t)rawHum / 1000.0;

      Serial.print("Humidity: ");
      Serial.print(humidity, 2);
      Serial.println(" %RH");
      Serial.println("--------------------");
    } else {
      Serial.print("Read Hum Failed! Error: 0x");
      Serial.println(resultHum, HEX);
      Serial.println("--------------------");
    }
  }
}