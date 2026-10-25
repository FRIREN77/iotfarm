#include <ModbusMaster.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ===== Wi-Fi =====
const char* ssid = "UVIRIAFAHEART";
const char* password = "nadia2018";

// ===== AWS MQTT =====
const char* mqttServer = "54.79.125.142";
const int mqttPort = 1883;
const char* mqttUser = "esp32user";
const char* mqttPassword = "root1234";
const char* mqttTopic = "s700/weather";

// ===== Modbus =====
#define RXD2 4
#define TXD2 15
#define DIR_PIN 2

ModbusMaster node;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastReadTime = 0;
const unsigned long readInterval = 3000;

unsigned long lastWiFiAttempt = 0;
const unsigned long wifiRetryInterval = 10000;

unsigned long lastMQTTAttempt = 0;
const unsigned long mqttRetryInterval = 5000;

// ===== RS-485 =====
void preTransmission() {
  digitalWrite(DIR_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(DIR_PIN, LOW);
}

// ===== Wi-Fi =====
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  lastWiFiAttempt = millis();
}

// ===== MQTT =====
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) {
    return;
  }

  lastMQTTAttempt = millis();

  Serial.print("Connecting MQTT... ");

  String clientId = "s700-" + WiFi.macAddress();
  clientId.replace(":", "");

  if (mqtt.connect(clientId.c_str(), mqttUser, mqttPassword)) {
    Serial.println("connected");
  } else {
    Serial.print("failed, state=");
    Serial.println(mqtt.state());
  }
}

// ===== Read Modbus =====
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
      ((uint32_t)node.getResponseBuffer(0) << 16)
      | node.getResponseBuffer(1);

  value = (int32_t)raw / 1000.0f;
  return true;
}

// ===== Read and display =====
bool getSensor(const char* name, uint16_t address, float &value) {
  if (!readSensor(address, value)) return false;

  Serial.print(name);
  Serial.print(": ");
  Serial.println(value, 2);

  return true;
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);

  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  node.begin(20, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.println("ESP32 Modbus Master Initialized");

  WiFi.mode(WIFI_STA);
  connectWiFi();

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setBufferSize(512);
}

// ===== Main Loop =====
void loop() {
  unsigned long now = millis();

  // Wi-Fi reconnect
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWiFiAttempt >= wifiRetryInterval) {
      connectWiFi();
    }
  }

  static bool wifiWasConnected = false;
  bool wifiConnected = WiFi.status() == WL_CONNECTED;

  if (wifiConnected && !wifiWasConnected) {
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    connectMQTT();
  }

  wifiWasConnected = wifiConnected;

  // MQTT reconnect
  if (wifiConnected && !mqtt.connected()) {
    if (now - lastMQTTAttempt >= mqttRetryInterval) {
      connectMQTT();
    }
  }

  if (mqtt.connected()) {
    mqtt.loop();
  }

  // Read S700 every 3 seconds
  if (now - lastReadTime >= readInterval) {
    lastReadTime = now;

    float temperature = 0;
    float humidity = 0;
    float pressure = 0;
    float light = 0;
    float windSpeed = 0;

    bool ok = true;

    ok &= getSensor("Temperature (C)", 0x0000, temperature);
    delay(100);

    ok &= getSensor("Humidity (%RH)", 0x0002, humidity);
    delay(100);

    ok &= getSensor("Barometric pressure (Pa)", 0x0004, pressure);
    delay(100);

    ok &= getSensor("Light intensity (lx)", 0x0006, light);
    delay(100);

    ok &= getSensor("Average wind speed (m/s)", 0x0012, windSpeed);

    Serial.println("--------------------");

    if (!ok) {
      Serial.println("Incomplete sensor data - MQTT skipped");
      return;
    }

    // Convert pressure Pa → kPa
    pressure /= 1000.0f;

    // Estimate solar radiation from lux
    float solarRadiation = light / 115.0f;

    Serial.print("Pressure: ");
    Serial.print(pressure, 2);
    Serial.println(" kPa");

    Serial.print("Estimated solar radiation: ");
    Serial.print(solarRadiation, 2);
    Serial.println(" W/m2");

    // ===== JSON =====
    char payload[300];

    snprintf(
      payload,
      sizeof(payload),
      "{\"temperature\":%.2f,"
      "\"humidity\":%.2f,"
      "\"pressure\":%.3f,"
      "\"light\":%.2f,"
      "\"wind_speed\":%.2f,"
      "\"solar_radiation\":%.2f}",
      temperature,
      humidity,
      pressure,
      light,
      windSpeed,
      solarRadiation
    );

    Serial.println(payload);

    // ===== Publish MQTT =====
    if (mqtt.connected()) {
      if (mqtt.publish(mqttTopic, payload)) {
        Serial.println("MQTT sent successfully");
      } else {
        Serial.println("MQTT publish failed");
      }
    } else {
      Serial.println("MQTT disconnected - data not sent");
    }
  }
}