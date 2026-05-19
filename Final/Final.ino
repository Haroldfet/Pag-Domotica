#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// CONFIGURACION
#define EEPROM_SIZE        512
#define SSID_ADDR          0
#define PASS_ADDR          65
#define CONFIG_FLAG_ADDR   130

// Pines sensores (ADC)
#define PANEL_VOLTAGE_PIN  35
#define PANEL_CURRENT_PIN  34
#define BATTERY_VOLTAGE_PIN 33
#define BATTERY_LEVEL_PIN  32

// Pines reles (GPIO)
#define LIGHT1_PIN 25
#define LIGHT2_PIN 26
#define LIGHT3_PIN 27

// VARIABLES GLOBALES
WebServer server(80);
String wifiSSID = "";
String wifiPassword = "";
bool wifiConfigured = false;

struct SystemStatus {
  float panelVoltage   = 0;
  float panelCurrent   = 0;
  float panelPower     = 0;
  float batteryVoltage = 0;
  int   batteryLevel   = 0;
  float energyToday    = 0;
  float energyMonth    = 0;
  bool  connected      = false;
  bool  luz1 = false;
  bool  luz2 = false;
  bool  luz3 = false;
} systemStatus;

// EEPROM
void loadWiFiConfig() {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(CONFIG_FLAG_ADDR) == 99) {
    wifiConfigured = true;
    wifiSSID = "";
    for (int i = 0; i < 64; i++) { char c = EEPROM.read(SSID_ADDR + i); if (c == '\0') break; wifiSSID += c; }
    wifiPassword = "";
    for (int i = 0; i < 64; i++) { char c = EEPROM.read(PASS_ADDR + i); if (c == '\0') break; wifiPassword += c; }
  }
}

void saveWiFiConfig(String ssid, String password) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < (int)ssid.length(); i++)      EEPROM.write(SSID_ADDR + i, ssid[i]);
  EEPROM.write(SSID_ADDR + ssid.length(), '\0');
  for (int i = 0; i < (int)password.length(); i++)  EEPROM.write(PASS_ADDR + i, password[i]);
  EEPROM.write(PASS_ADDR + password.length(), '\0');
  EEPROM.write(CONFIG_FLAG_ADDR, 99);
  EEPROM.commit();
}

void eraseWiFiConfig() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();
}

// SENSORES
void readSensors() {
  int pvRaw  = analogRead(PANEL_VOLTAGE_PIN);
  int pcRaw  = analogRead(PANEL_CURRENT_PIN);
  int bvRaw  = analogRead(BATTERY_VOLTAGE_PIN);
  int blRaw  = analogRead(BATTERY_LEVEL_PIN);

  systemStatus.panelVoltage   = (pvRaw  / 4095.0f) * 100.0f;
  systemStatus.panelCurrent   = (pcRaw  / 4095.0f) * 50.0f;
  systemStatus.panelPower     = systemStatus.panelVoltage * systemStatus.panelCurrent;
  systemStatus.batteryVoltage = (bvRaw  / 4095.0f) * 20.0f;
  systemStatus.batteryLevel   = map(blRaw, 0, 4095, 0, 100);
  systemStatus.connected      = (systemStatus.panelVoltage > 5.0f);
  systemStatus.energyToday    = systemStatus.panelPower * 0.0002f;
  systemStatus.energyMonth    = systemStatus.energyToday * 0.3f;
}

void toggleLight(int id) {
  switch (id) {
    case 1: systemStatus.luz1 = !systemStatus.luz1; digitalWrite(LIGHT1_PIN, systemStatus.luz1 ? HIGH : LOW); break;
    case 2: systemStatus.luz2 = !systemStatus.luz2; digitalWrite(LIGHT2_PIN, systemStatus.luz2 ? HIGH : LOW); break;
    case 3: systemStatus.luz3 = !systemStatus.luz3; digitalWrite(LIGHT3_PIN, systemStatus.luz3 ? HIGH : LOW); break;
  }
}

// PAGINAS HTML EN PROGMEM
#include "html_pages.h"

// MANEJADORES DE RUTAS
void handleRoot() {
  server.send_P(200, "text/html", wifiConfigured ? INIT_HTML : CONFIG_HTML);
}

void handleSaveConfig() {
  if (server.method() != HTTP_POST) { server.send(405, "text/plain", "Method Not Allowed"); return; }
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() == 0 || pass.length() == 0) { server.send(400, "text/plain", "SSID y contrasena requeridos"); return; }
  saveWiFiConfig(ssid, pass);
  wifiSSID = ssid; wifiPassword = pass; wifiConfigured = true;
  server.send(200, "text/html", "<h1>Configuracion guardada. Reiniciando...</h1><script>setTimeout(()=>location.href='/',2000)</script>");
  delay(1000);
  ESP.restart();
}

void handleGetDatos() {
  readSensors();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  DynamicJsonDocument doc(512);
  doc["panelVoltage"]   = systemStatus.panelVoltage;
  doc["panelCurrent"]   = systemStatus.panelCurrent;
  doc["panelPower"]     = systemStatus.panelPower;
  doc["batteryVoltage"] = systemStatus.batteryVoltage;
  doc["batteryLevel"]   = systemStatus.batteryLevel;
  doc["energyToday"]    = systemStatus.energyToday;
  doc["energyMonth"]    = systemStatus.energyMonth;
  doc["connected"]      = systemStatus.connected;
  doc["luz1"]           = systemStatus.luz1;
  doc["luz2"]           = systemStatus.luz2;
  doc["luz3"]           = systemStatus.luz3;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleToggleLuz() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String uri = server.uri();
  int id = uri.substring(uri.lastIndexOf('/') + 1).toInt();
  if (id >= 1 && id <= 3) {
    toggleLight(id);
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "text/plain", "ID invalido");
  }
}

void handleForgetWiFi() {
  server.send_P(200, "text/html", RESET_HTML);
  delay(2000);
  eraseWiFiConfig();
  delay(500);
  ESP.restart();
}

// CONEXION WIFI
void connectToWiFi() {
  if (!wifiConfigured) {
    Serial.println("[INFO] WiFi no configurado. Modo AP activado.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("EcoSolar", "12345678");
    Serial.print("[INFO] AP IP: "); Serial.println(WiFi.softAPIP());
    return;
  }
  Serial.print("[INFO] Conectando a: "); Serial.println(wifiSSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { delay(500); Serial.print("."); attempts++; }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(); Serial.print("[OK] IP: "); Serial.println(WiFi.localIP());
    systemStatus.connected = true;
  } else {
    Serial.println(); Serial.println("[ERROR] Fallo WiFi. Modo AP activado.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("EcoSolar", "12345678");
  }
}

// SETUP
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== ECOSOLAR ESP32 ===");

  pinMode(LIGHT1_PIN, OUTPUT); digitalWrite(LIGHT1_PIN, LOW);
  pinMode(LIGHT2_PIN, OUTPUT); digitalWrite(LIGHT2_PIN, LOW);
  pinMode(LIGHT3_PIN, OUTPUT); digitalWrite(LIGHT3_PIN, LOW);

  loadWiFiConfig();
  connectToWiFi();

  server.on("/",                        handleRoot);
  server.on("/guardar",                 handleSaveConfig);
  server.on("/reset",         HTTP_GET, [](){ server.send_P(200, "text/html", RESET_HTML); });
  server.on("/api/datos",               handleGetDatos);
  server.on("/api/luz/1/toggle", HTTP_POST, handleToggleLuz);
  server.on("/api/luz/2/toggle", HTTP_POST, handleToggleLuz);
  server.on("/api/luz/3/toggle", HTTP_POST, handleToggleLuz);
  server.on("/api/forget-wifi",  HTTP_POST, handleForgetWiFi);

  server.begin();
  Serial.println("[INFO] Servidor iniciado en puerto 80");
  Serial.println("======================\n");
}

// LOOP
void loop() {
  server.handleClient();
  delay(10);
}
