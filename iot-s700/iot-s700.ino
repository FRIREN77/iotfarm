#include <ModbusMaster.h>

#define RXD2 4
#define TXD2 15

ModbusMaster node;

unsigned long lastReadTime = 0;
const long readInterval = 5000; // อ่านทุกๆ 5 วินาที

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  node.begin(20, Serial2); // Slave ID 20
  
  Serial.println("ESP32 Modbus Master Initialized.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;

    // สมมติว่าตอนนี้เราอ่านมาสัก 4 Registers (0x0000 ถึง 0x0003) 
    // โดย 0x0000-0x0001 เป็น Temperature และ 0x0002-0x0003 น่าจะเป็น Humidity (หรือตัวถัดไป)
    uint8_t result = node.readInputRegisters(0x0000, 4);

    if (result == node.ku8MBSuccess) {
      Serial.println("\n--- Read Success ---");

      // แปลง Register 0 และ 1 ให้เป็น Float (Temperature)
      union {
        float f;
        uint16_t raw[2];
      } tempVal;

      // จัดเรียง High/Low word (ถ้าค่าเพี้ยนลองสลับ index 0 กับ 1 ดูครับ)
      tempVal.raw[1] = node.getResponseBuffer(0); 
      tempVal.raw[0] = node.getResponseBuffer(1); 
      float temp = tempVal.f;

      // แปลง Register 2 และ 3 ให้เป็น Float (Humidity - สมมติว่าอยู่ถัดไป)
      union {
        float f;
        uint16_t raw[2];
      } humVal;

      humVal.raw[1] = node.getResponseBuffer(2); 
      humVal.raw[0] = node.getResponseBuffer(3); 
      float humidity = humVal.f;

      Serial.print("Temperature: "); Serial.print(temp); Serial.println(" °C");
      Serial.print("Humidity:    "); Serial.print(humidity); Serial.println(" %RH");
      Serial.println("--------------------");

    } else {
      Serial.print("Read Failed! Error Code: 0x");
      Serial.println(result, HEX);
    }
  }
}