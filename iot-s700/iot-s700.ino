
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

// Read signed 32-bit value from two Big Endian registers
bool readSensor(uint16_t address, float &value) {
  uint8_t result = node.readInputRegisters(address, 2);

  if (result != node.ku8MBSuccess) {
    Serial.print("Read failed at 0x");
    Serial.print(address, HEX);
    Serial.print(" Error: 0x");
    Serial.println(result, HEX);
    return false;
  }

  uint32_t raw =
      ((uint32_t)node.getResponseBuffer(0) * 65536UL)
      + node.getResponseBuffer(1);

  value = (int32_t)raw / 1000.0;

  return true;
}

void printSensor(const char *name, uint16_t address) {
  float value;

  if (readSensor(address, value)) {
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value, 2);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  node.begin(60, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.println("ESP32 Modbus Master Initialized");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;

    // Temperature
    printSensor("Temperature (C)", 0x0000);
    delay(100);

    // Humidity
    printSensor("Humidity (%RH)", 0x0002);
    delay(100);

    // Barometric pressure
    printSensor("Barometric pressure", 0x0004);
    delay(100);

    // Light intensity
    printSensor("Light intensity", 0x0006);
    delay(100);

    // Average wind speed
    printSensor("Average wind speed", 0x0012);
    delay(100);

    // Global solar radiation
    printSensor("Global solar radiation", 0x004A);

    Serial.println("--------------------");
  }
}
