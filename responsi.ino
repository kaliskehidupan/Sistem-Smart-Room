
/*
  =========================================================
  SMART ROOM MONITORING SYSTEM
  Responsi 1 Internet of Things 2026/2027
  Universitas 'Aisyiyah Yogyakarta

  Hardware:
  - ESP32 DevKit V1
  - DHT22
  - PIR HC-SR501
  - Ultrasonic HC-SR04
  - 2 LED + resistor
  =========================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// =========================================================
// 1. KONFIGURASI WIFI
// =========================================================

const char* WIFI_SSID = "ESP32_TEST";
const char* WIFI_PASSWORD = "12345678";

WebServer server(80);

unsigned long lastReconnect = 0;
const unsigned long RECONNECT_INTERVAL = 5000;

// =========================================================
// 2. KONFIGURASI PIN
// =========================================================

#define DHT_PIN 4
#define DHT_TYPE DHT22

#define PIR_PIN 19

#define TRIG_PIN 5
#define ECHO_PIN 18

#define LED_RED 25
#define LED_GREEN 26

DHT dht(DHT_PIN, DHT_TYPE);

// =========================================================
// 3. VARIABEL SENSOR
// =========================================================

float temperature = NAN;
float humidity = NAN;
float distanceCm = -1;

int motion = LOW;

String roomStatus = "INITIALIZING";

unsigned long lastDHTRead = 0;
unsigned long lastUltrasonicRead = 0;
unsigned long lastSerialPrint = 0;

const unsigned long DHT_INTERVAL = 2500;
const unsigned long ULTRASONIC_INTERVAL = 500;
const unsigned long SERIAL_INTERVAL = 2000;

// Ambang batas yang dapat disesuaikan
const float MAX_TEMPERATURE = 30.0;
const float NEAR_DISTANCE = 50.0;

// =========================================================
// 4. MEMBACA SENSOR ULTRASONIC
// =========================================================

float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // Timeout 25 ms agar program tidak menunggu terlalu lama
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0) {
    return -1;
  }

  // Konversi waktu pantul menjadi jarak dalam cm
  return duration / 58.0;
}

// =========================================================
// 5. PROSES DAN KLASIFIKASI KONDISI RUANGAN
// =========================================================

void processRoomStatus() {

  bool highTemperature =
    !isnan(temperature) && temperature >= MAX_TEMPERATURE;

  bool objectNearby =
    distanceCm > 0 && distanceCm <= NEAR_DISTANCE;

  bool motionDetected = (motion == HIGH);

  if (motionDetected || highTemperature || objectNearby) {

    roomStatus = "WARNING";

    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);

  } else {

    roomStatus = "NORMAL";

    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }
}

// =========================================================
// 6. MEMBACA DAN MEMPERBARUI DATA SENSOR
// =========================================================

void updateSensors() {

  unsigned long now = millis();

  // Membaca DHT22 setiap 2,5 detik
  if (now - lastDHTRead >= DHT_INTERVAL) {

    lastDHTRead = now;

    float newTemperature = dht.readTemperature();
    float newHumidity = dht.readHumidity();

    if (!isnan(newTemperature) && !isnan(newHumidity)) {

      temperature = newTemperature;
      humidity = newHumidity;

    } else {

      Serial.println("[ERROR] Gagal membaca DHT22!");
    }
  }

  // Membaca sensor ultrasonik setiap 500 ms
  if (now - lastUltrasonicRead >= ULTRASONIC_INTERVAL) {

    lastUltrasonicRead = now;
    distanceCm = readDistance();
  }

  // Membaca sensor PIR
  motion = digitalRead(PIR_PIN);

  // Memproses seluruh data sensor
  processRoomStatus();
}

// =========================================================
// 7. MENAMPILKAN DATA KE SERIAL MONITOR
// =========================================================

void printSensorData() {

  unsigned long now = millis();

  if (now - lastSerialPrint >= SERIAL_INTERVAL) {

    lastSerialPrint = now;

    Serial.println();
    Serial.println("====================================");
    Serial.println("       SMART ROOM MONITORING");
    Serial.println("====================================");

    Serial.print("Temperature : ");

    if (isnan(temperature)) {
      Serial.println("INVALID");
    } else {
      Serial.print(temperature, 1);
      Serial.println(" C");
    }

    Serial.print("Humidity    : ");

    if (isnan(humidity)) {
      Serial.println("INVALID");
    } else {
      Serial.print(humidity, 1);
      Serial.println(" %");
    }

    Serial.print("Motion      : ");
    Serial.println(motion == HIGH ? "DETECTED" : "NO MOTION");

    Serial.print("Distance    : ");

    if (distanceCm < 0) {
      Serial.println("NO ECHO");
    } else {
      Serial.print(distanceCm, 1);
      Serial.println(" cm");
    }

    Serial.print("Room Status : ");
    Serial.println(roomStatus);

    Serial.print("WiFi Status : ");
    Serial.println(WiFi.status() == WL_CONNECTED
                   ? "CONNECTED" : "DISCONNECTED");

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP Address  : ");
      Serial.println(WiFi.localIP());

      Serial.print("RSSI        : ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }

    Serial.println("====================================");
  }
}

// =========================================================
// 8. HALAMAN DASHBOARD WEB
// =========================================================

void handleRoot() {

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>Smart Room Monitoring</title>

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, sans-serif;
  background: #101827;
  color: white;
  padding: 20px;
}

.container {
  max-width: 850px;
  margin: auto;
}

h1 {
  color: #38bdf8;
}

.subtitle {
  color: #aab7c5;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
  gap: 15px;
  margin-top: 25px;
}

.card {
  background: #1e293b;
  border-radius: 12px;
  padding: 22px;
}

.card h3 {
  color: #94a3b8;
  margin-top: 0;
}

.value {
  font-size: 30px;
  font-weight: bold;
}

.status {
  margin-top: 20px;
  padding: 22px;
  border-radius: 12px;
  text-align: center;
  font-size: 22px;
  font-weight: bold;
}

.normal {
  background: #14532d;
  color: #86efac;
}

.warning {
  background: #7f1d1d;
  color: #fca5a5;
}

.footer {
  margin-top: 25px;
  color: #94a3b8;
  font-size: 13px;
}

</style>
</head>

<body>

<div class="container">

<h1>SMART ROOM MONITORING</h1>

<p class="subtitle">
ESP32 Internet of Things Dashboard
</p>

<div class="grid">

<div class="card">
<h3>Temperature</h3>
<div class="value" id="temp">-- °C</div>
</div>

<div class="card">
<h3>Humidity</h3>
<div class="value" id="hum">-- %</div>
</div>

<div class="card">
<h3>Motion Detection</h3>
<div class="value" id="motion">--</div>
</div>

<div class="card">
<h3>Object Distance</h3>
<div class="value" id="distance">-- cm</div>
</div>

</div>

<div class="status normal" id="status">
CONNECTING...
</div>

<div class="footer">
<p>WiFi: <span id="wifi">--</span></p>
<p>RSSI: <span id="rssi">--</span></p>
<p>Last Update: <span id="updated">--</span></p>
<p>ESP32 Smart Room Monitoring System</p>
</div>

</div>

<script>

async function updateDashboard() {

  try {

    const response = await fetch('/api');
    if (!response.ok) throw new Error('HTTP Error');

    const data = await response.json();

    document.getElementById('temp').textContent =
      data.temperature === null ? 'INVALID' :
      data.temperature + ' °C';

    document.getElementById('hum').textContent =
      data.humidity === null ? 'INVALID' :
      data.humidity + ' %';

    document.getElementById('motion').textContent =
      data.motion ? 'DETECTED' : 'NO MOTION';

    document.getElementById('distance').textContent =
      data.distance === null ? 'NO ECHO' :
      data.distance + ' cm';

    const status = document.getElementById('status');

    status.textContent = data.status;
    status.className = 'status ' +
      (data.status === 'NORMAL' ? 'normal' : 'warning');

    document.getElementById('wifi').textContent =
      data.wifi;

    document.getElementById('rssi').textContent =
      data.rssi + ' dBm';

    document.getElementById('updated').textContent =
      new Date().toLocaleTimeString();

  } catch(error) {

    document.getElementById('status').textContent =
      'CONNECTION LOST';

    document.getElementById('status').className =
      'status warning';
  }
}

updateDashboard();
setInterval(updateDashboard, 1500);

</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// =========================================================
// 9. API DATA SENSOR DALAM FORMAT JSON
// =========================================================

void handleAPI() {

  char json[256];

  char tempText[16];
  char humidityText[16];
  char distanceText[16];

  if (isnan(temperature)) {
    strcpy(tempText, "null");
  } else {
    snprintf(tempText, sizeof(tempText), "%.1f", temperature);
  }

  if (isnan(humidity)) {
    strcpy(humidityText, "null");
  } else {
    snprintf(humidityText, sizeof(humidityText), "%.1f", humidity);
  }

  if (distanceCm < 0) {
    strcpy(distanceText, "null");
  } else {
    snprintf(distanceText, sizeof(distanceText), "%.1f", distanceCm);
  }

  snprintf(
    json,
    sizeof(json),
    "{\"temperature\":%s,\"humidity\":%s,"
    "\"motion\":%s,\"distance\":%s,"
    "\"status\":\"%s\",\"wifi\":\"%s\",\"rssi\":%d}",
    tempText,
    humidityText,
    motion == HIGH ? "true" : "false",
    distanceText,
    roomStatus.c_str(),
    WiFi.SSID().c_str(),
    WiFi.RSSI()
  );

  server.send(200, "application/json", json);
}

// =========================================================
// 10. SETUP
// =========================================================

void setup() {

  Serial.begin(115200);

  // Konfigurasi pin sensor
  pinMode(PIR_PIN, INPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Konfigurasi LED
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  // Inisialisasi DHT22
  dht.begin();

  Serial.println();
  Serial.println("SMART ROOM MONITORING SYSTEM");
  Serial.println("Initializing sensors...");

  // Mengaktifkan WiFi Station Mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 20000) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("[SUCCESS] WiFi Connected!");

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

  } else {

    Serial.println("[WARNING] WiFi connection failed.");
    Serial.println("Automatic reconnect will be attempted.");
  }

  // Konfigurasi alamat dashboard
  server.on("/", handleRoot);
  server.on("/api", handleAPI);

  // Menjalankan web server
  server.begin();

  Serial.println("Web Server Started!");
}

// =========================================================
// 11. LOOP
// =========================================================

void loop() {

  // Memperbarui data sensor
  updateSensors();

  // Menampilkan hasil ke Serial Monitor
  printSensorData();

  // Melayani permintaan browser
  if (WiFi.status() == WL_CONNECTED) {

    server.handleClient();

  } else {

    // Auto reconnect setiap 5 detik
    if (millis() - lastReconnect >= RECONNECT_INTERVAL) {

      lastReconnect = millis();

      Serial.println("[WiFi] Reconnecting...");

      WiFi.reconnect();
    }
  }
}
