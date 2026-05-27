#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <DHT.h>

// =====================================================
// WIFI CONFIGURATION
// =====================================================

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// =====================================================
// THINGSPEAK API KEY
// =====================================================

String apiKey = "SBBAMMJSIS09ZN0H";

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

// Number of consecutive abnormal readings required
const int faultLimit = 3;

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // =====================================================
  // START DHT22
  // =====================================================

  dht.begin();

  // =====================================================
  // START MPU6050
  // =====================================================

  if (!mpu.begin()) {

    Serial.println("MPU6050 not found!");

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

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");

  Serial.println("=====================================");
  Serial.println("Industrial Motor Monitoring Started");
  Serial.println("=====================================");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // =====================================================
  // READ DHT22
  // =====================================================

  float temperature = dht.readTemperature();

  // =====================================================
  // READ POTENTIOMETER
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
  // MOTOR STATUS VARIABLE
  // =====================================================

  int motorStatus = 0;

  // =====================================================
  // DEFINE CONDITIONS
  // =====================================================

  bool warningCondition =
    (temperature > 50 ||
     rpm > 4000 ||
     abs(vibrationX) > 8);

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

      Serial.println("CRITICAL MOTOR FAULT CONFIRMED!");
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

      Serial.println("WARNING CONDITION CONFIRMED!");
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
  // DISPLAY SENSOR VALUES
  // =====================================================

  Serial.println("=====================================");

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

    http.begin(url);

    int httpResponseCode = http.GET();

    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    http.end();
  }

  else {

    Serial.println("WiFi Disconnected");
  }

  Serial.println("");

  // =====================================================
  // THINGSPEAK UPDATE DELAY
  // =====================================================

  delay(15000);
}
