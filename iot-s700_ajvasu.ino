#include <ModbusMaster.h>

#define RXD2 4
#define TXD2 15
#define DIR_PIN 2  // ตามไดอะแกรม ขา DIR ต่ออยู่กับ IO2

ModbusMaster node;

// ฟังก์ชันสั่งเปิดโหมดส่ง (Transmit) ก่อนส่งข้อมูลออกไป
void preTransmission() {
  digitalWrite(DIR_PIN, HIGH);
}

// ฟังก์ชันสั่งปิดโหมดส่ง กลับเป็นโหมดรับ (Receive) เมื่อส่งเสร็จ
void postTransmission() {
  digitalWrite(DIR_PIN, LOW);
}

unsigned long lastReadTime = 0;
const long readInterval = 3000; // อ่านทุกๆ 3 วินาที

void setup() {
  Serial.begin(115200);
  
  // ตั้งค่าขา DIR เป็น Output และตั้งค่าเริ่มต้นให้อยู่โหมดรับ (LOW)
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);

  // เริ่มต้น Serial2
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  node.begin(20, Serial2); // Slave ID 20
  
  // ลงทะเบียนฟังก์ชันควบคุมทิศทางให้กับ ModbusMaster
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  
  Serial.println("ESP32 Modbus Master Initialized with DIR Control on IO2.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;

    // --- 1. อ่านค่าอุณหภูมิ (2 Registers เริ่มที่ 0x0000) ---
    uint8_t resultTemp = node.readInputRegisters(0x0000, 2);
    
    if (resultTemp == node.ku8MBSuccess) {
      union {
        float f;
        uint16_t raw[2];
      } tempVal;

      tempVal.raw[1] = node.getResponseBuffer(0); 
      tempVal.raw[0] = node.getResponseBuffer(1); 
      float temp = tempVal.f;

      Serial.print("Temperature: "); 
      Serial.print(tampval.raw[1]*256 + tampval.raw[0])); 
      Serial.println(" °C");
    } else {
      Serial.print("Read Temp Failed! Error: 0x");
      Serial.println(resultTemp, HEX);
    }

    // หน่วงเวลาสั้นๆ ระหว่างคำสั่ง
    delay(100);

    // --- 2. อ่านค่าความชื้น (2 Registers ถัดมา เริ่มที่ 0x0002) ---
    uint8_t resultHum = node.readInputRegisters(0x0002, 2);
    
    if (resultHum == node.ku8MBSuccess) {
      union {
        float f;
        uint16_t raw[2];
      } humVal;

      humVal.raw[1] = node.getResponseBuffer(0); 
      humVal.raw[0] = node.getResponseBuffer(1); 
      float humidity = humVal.f;

      Serial.print("Humidity:    "); 
      Serial.print(humVal.raw[1]*256 + humVal.raw[0]); 
      Serial.println(" %RH");
      Serial.println("--------------------");
    } else {
      Serial.print("Read Hum Failed! Error: 0x");
      Serial.println(resultHum, HEX);
      Serial.println("--------------------");
    }
  }
}