#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "DHTesp.h"
#include <ESP32Servo.h>

// =======================
// PIN SENSOR
// =======================
#define DHT_PIN 15
#define LDR_PIN 34
#define PIR_PIN 27

// =======================
// PIN ACTUATOR
// =======================
#define LED_GREEN_PIN 18
#define LED_YELLOW_PIN 19
#define LED_RED_PIN 21
#define SERVO_PIN 22
#define BUZZER_PIN 23

// =======================
// WIFI WOKWI
// =======================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// =======================
// THINGSPEAK
// =======================
const char* RAW_WRITE_API_KEY = "YOUR_RAW_WRITE_API_KEY";

const unsigned long AI_CHANNEL_ID = 1234567;
const char* AI_READ_API_KEY = "YOUR_AI_READ_API_KEY";

// =======================
// OBJECTS
// =======================
DHTesp dht;
Servo ventilationServo;

// Free ThingSpeak: gunakan interval >= 15 detik.
// 20 detik memberi margin aman.
const unsigned long SENSOR_SEND_INTERVAL = 20000;
const unsigned long AI_POLL_INTERVAL = 10000;

unsigned long lastSensorSend = 0;
unsigned long lastAIPoll = 0;
int lastAIStatus = -1;

// =======================
// WIFI
// =======================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =======================
// ACTUATOR
// =======================
void setLED(bool green, bool yellow, bool red) {
  digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(LED_YELLOW_PIN, yellow ? HIGH : LOW);
  digitalWrite(LED_RED_PIN, red ? HIGH : LOW);
}

void actuatorNormal() {
  setLED(true, false, false);
  noTone(BUZZER_PIN);
  ventilationServo.write(0);

  Serial.println("ACTION: NORMAL");
  Serial.println("Green LED ON | Buzzer OFF | Servo 0 deg");
}

void actuatorWarning() {
  setLED(false, true, false);
  tone(BUZZER_PIN, 1200, 300);
  ventilationServo.write(45);

  Serial.println("ACTION: WARNING");
  Serial.println("Yellow LED ON | Beep | Servo 45 deg");
}

void actuatorCritical() {
  setLED(false, false, true);
  tone(BUZZER_PIN, 2000);
  ventilationServo.write(90);

  Serial.println("ACTION: CRITICAL");
  Serial.println("Red LED ON | Alarm ON | Servo 90 deg");
}

void applyAIStatus(int status) {
  Serial.println();
  Serial.println("===== CLOUD NN RESULT =====");

  switch (status) {
    case 0:
      Serial.println("AI STATUS: NORMAL");
      actuatorNormal();
      break;

    case 1:
      Serial.println("AI STATUS: WARNING");
      actuatorWarning();
      break;

    case 2:
      Serial.println("AI STATUS: CRITICAL");
      actuatorCritical();
      break;

    default:
      Serial.println("Unknown AI Status");
      break;
  }

  Serial.println("===========================");
}

// =======================
// CLOUD -> ESP32
// Field 4 = AI Status
// =======================
int readCloudAIStatus() {
  connectWiFi();

  WiFiClientSecure client;
  client.setInsecure(); // untuk demo/simulator

  HTTPClient http;

  String url =
    "https://api.thingspeak.com/channels/" +
    String(AI_CHANNEL_ID) +
    "/fields/4/last.txt?api_key=" +
    String(AI_READ_API_KEY);

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.print("AI HTTP Error: ");
    Serial.println(httpCode);
    http.end();
    return -1;
  }

  String payload = http.getString();
  payload.trim();
  http.end();

  if (payload.length() == 0) return -1;

  int status = payload.toInt();
  if (status < 0 || status > 2) return -1;

  Serial.print("Cloud AI Status: ");
  Serial.println(status);

  return status;
}

// =======================
// ESP32 -> CLOUD
// =======================
void sendSensorData(
  float temperature,
  float humidity,
  int light,
  int motion
) {
  connectWiFi();

  WiFiClientSecure client;
  client.setInsecure(); // untuk demo/simulator

  HTTPClient http;

  String url = "https://api.thingspeak.com/update";
  url += "?api_key=" + String(RAW_WRITE_API_KEY);
  url += "&field1=" + String(temperature, 2);
  url += "&field2=" + String(humidity, 2);
  url += "&field3=" + String(light);
  url += "&field4=" + String(motion);

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String response = http.getString();

    Serial.print("ThingSpeak HTTP: ");
    Serial.println(httpCode);

    Serial.print("Entry ID: ");
    Serial.println(response);
  } else {
    Serial.print("ThingSpeak Error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void readAndSendSensor() {
  TempAndHumidity data = dht.getTempAndHumidity();
  int light = analogRead(LDR_PIN);
  int motion = digitalRead(PIR_PIN);

  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("DHT READ ERROR");
    return;
  }

  Serial.println();
  Serial.println("===== SENSOR =====");
  Serial.printf("Temperature : %.2f C\n", data.temperature);
  Serial.printf("Humidity    : %.2f %%\n", data.humidity);
  Serial.printf("Light ADC   : %d\n", light);
  Serial.printf("Motion      : %d\n", motion);

  sendSensorData(
    data.temperature,
    data.humidity,
    light,
    motion
  );
}

void setup() {
  Serial.begin(115200);

  dht.setup(DHT_PIN, DHTesp::DHT22);

  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  ventilationServo.setPeriodHertz(50);
  ventilationServo.attach(SERVO_PIN, 500, 2400);

  setLED(false, false, false);
  noTone(BUZZER_PIN);
  ventilationServo.write(0);

  connectWiFi();

  Serial.println();
  Serial.println("AIoT CLOUD NEURAL NETWORK READY");
}

void loop() {
  unsigned long now = millis();

  // SENSOR -> THINGSPEAK RAW
  if (now - lastSensorSend >= SENSOR_SEND_INTERVAL) {
    lastSensorSend = now;
    readAndSendSensor();
  }

  // THINGSPEAK AI RESULT -> ESP32
  if (now - lastAIPoll >= AI_POLL_INTERVAL) {
    lastAIPoll = now;

    int aiStatus = readCloudAIStatus();

    // Aktuator hanya di-update saat status berubah.
    if (aiStatus >= 0 && aiStatus != lastAIStatus) {
      lastAIStatus = aiStatus;
      applyAIStatus(aiStatus);
    }
  }
}
