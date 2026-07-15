#include <WiFi.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <DHT.h>

// =====================================================
// WIFI CONFIGURATION
// =====================================================

// For Wokwi Simulation
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// =====================================================
// THINGSPEAK API KEY
// =====================================================
// ADD your thingspeak APIKEY (get one from thingspeak.com > your channel > API Keys)
String apiKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define DHTPIN 15
#define DHTTYPE DHT22

#define POTPIN 34
#define LEDPIN 19
#define BUZZERPIN 18

// =====================================================
// SENSOR OBJECTS
// =====================================================

DHT dht(DHTPIN, DHTTYPE);

Adafruit_MPU6050 mpu;

// =====================================================
// FAULT COUNTERS
// =====================================================

int warningCounter = 0;
int criticalCounter = 0;

const int faultLimit = 3;

// =====================================================
// SETUP
// =====================================================

void setup() {

  // Start Serial Monitor
  Serial.begin(115200);

  delay(1000);

  Serial.println("==================================");
  Serial.println("SYSTEM STARTING...");
  Serial.println("==================================");

  // =====================================================
  // START DHT22
  // =====================================================

  dht.begin();

  Serial.println("DHT22 Initialized");

  // =====================================================
  // START MPU6050
  // =====================================================

  if (!mpu.begin()) {

    Serial.println("ERROR: MPU6050 NOT FOUND");

    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");

  // =====================================================
  // OUTPUT PINS
  // =====================================================

  pinMode(LEDPIN, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);

  // =====================================================
  // CONNECT TO WIFI
  // =====================================================

  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  int wifiTimeout = 0;

  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {

    delay(500);
    Serial.print(".");

    wifiTimeout++;
  }

  // =====================================================
  // WIFI STATUS
  // =====================================================

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("");
    Serial.println("WiFi Connected Successfully!");

  } else {

    Serial.println("");
    Serial.println("WiFi Connection Failed");
  }

  Serial.println("==================================");
  Serial.println("Industrial Motor Monitoring Active");
  Serial.println("==================================");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  Serial.println("");
  Serial.println("========== NEW CYCLE ==========");

  // =====================================================
  // READ TEMPERATURE
  // =====================================================

  float temperature = dht.readTemperature();

  if (isnan(temperature)) {

    Serial.println("DHT22 Read Failed");
    return;
  }

  // =====================================================
  // READ RPM FROM POTENTIOMETER
  // =====================================================

  int potValue = analogRead(POTPIN);

  int rpm = map(potValue, 0, 4095, 0, 5000);

  // =====================================================
  // READ MPU6050
  // =====================================================

  sensors_event_t a, g, tempSensor;

  mpu.getEvent(&a, &g, &tempSensor);

  float vibrationX = a.acceleration.x;
  float vibrationY = a.acceleration.y;
  float vibrationZ = a.acceleration.z;

  // =====================================================
  // MOTOR STATUS
  // =====================================================

  int motorStatus = 0;

  // 0 = Healthy
  // 1 = Warning
  // 2 = Critical

  // =====================================================
  // WARNING CONDITION
  // =====================================================

  bool warningCondition =
    (temperature > 50 ||
     rpm > 4000 ||
     abs(vibrationX) > 8);

  // =====================================================
  // CRITICAL CONDITION
  // =====================================================

  bool criticalCondition =
    (temperature > 70 ||
     rpm > 4500 ||
     abs(vibrationX) > 10);

  // =====================================================
  // CRITICAL FAULT LOGIC
  // =====================================================

  if (criticalCondition) {

    criticalCounter++;

    warningCounter = 0;

    Serial.print("Critical Counter: ");
    Serial.println(criticalCounter);

    if (criticalCounter >= faultLimit) {

      motorStatus = 2;

      digitalWrite(BUZZERPIN, HIGH);
      digitalWrite(LEDPIN, LOW);

      Serial.println("CRITICAL MOTOR FAULT!");
    }
  }

  // =====================================================
  // WARNING LOGIC
  // =====================================================

  else if (warningCondition) {

    warningCounter++;

    criticalCounter = 0;

    Serial.print("Warning Counter: ");
    Serial.println(warningCounter);

    if (warningCounter >= faultLimit) {

      motorStatus = 1;

      digitalWrite(BUZZERPIN, LOW);
      digitalWrite(LEDPIN, HIGH);

      Serial.println("WARNING CONDITION DETECTED");
    }
  }

  // =====================================================
  // NORMAL CONDITION
  // =====================================================

  else {

    warningCounter = 0;
    criticalCounter = 0;

    motorStatus = 0;

    digitalWrite(BUZZERPIN, LOW);
    digitalWrite(LEDPIN, HIGH);

    Serial.println("Motor Running Normally");
  }

  // =====================================================
  // DISPLAY VALUES
  // =====================================================

  Serial.println("----------------------------------");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("RPM: ");
  Serial.println(rpm);

  Serial.print("Vibration X: ");
  Serial.println(vibrationX);

  Serial.print("Vibration Y: ");
  Serial.println(vibrationY);

  Serial.print("Vibration Z: ");
  Serial.println(vibrationZ);

  Serial.print("Motor Status: ");
  Serial.println(motorStatus);

  // =====================================================
  // SEND DATA TO THINGSPEAK
  // =====================================================

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String url =
      "http://api.thingspeak.com/update?api_key=" + apiKey +
      "&field1=" + String(temperature) +
      "&field2=" + String(rpm) +
      "&field3=" + String(motorStatus) +
      "&field4=" + String(vibrationX) +
      "&field5=" + String(vibrationY);

    Serial.println("----------------------------------");
    Serial.println("Uploading Data to ThinkSpeak...");

    http.begin(url);

    int httpResponseCode = http.GET();

    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    http.end();
  }

  else {

    Serial.println("WiFi Not Connected");
  }

  Serial.println("==================================");

  // =====================================================
  // THINGSPEAK DELAY
  // =====================================================

  delay(15000);
}
