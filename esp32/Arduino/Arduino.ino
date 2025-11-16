#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <BH1750.h>
#include <ArduinoJson.h>

// ========== CONFIGURACIÓN WIFI ==========
const char* ssid = "Xiaomi 11T Pro";
const char* password = "Marton12";

// ========== PINES ==========
#define MOSFET_PIN 25
#define PIN_PIR 27
#define PIR_DEBUG_LED 2
#define SDA_PIN 21
#define SCL_PIN 22

// ========== CONFIGURACIÓN PWM ==========
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;

// ========== UMBRALES DE LUZ ==========
const float UMBRAL_LUZ_MIN = 50.0;
const float UMBRAL_LUZ_MAX = 400.0;
#define TIMEOUT_AUTO 3000

// ========== OBJETOS ==========
WebServer server(80);
Adafruit_INA219 ina219;
BH1750 lightMeter;

// ========== VARIABLES GLOBALES ==========
enum Modo { MANUAL, AUTOMATICO };
Modo modoActual = AUTOMATICO;

bool movimientoDetectado = false;
unsigned long tiempoUltimoMovimiento = 0;
int estadoPIRAnterior = LOW;
int brillo_actual = 0;
int brillo_manual = 128;

// Mediciones
float voltaje_shunt = 0.0;
float voltaje_bus = 0.0;
float corriente = 0.0;
float potencia = 0.0;
float energia_acumulada = 0.0;
unsigned long ultimaMedicion = 0;

float lux_actual = 0.0;
String nivel_luz_desc = "Inicializando...";

bool ina219_disponible = false;
bool bh1750_disponible = false;

// ========== FUNCIONES ==========
void setBrightness(int brillo) {
  brillo = constrain(brillo, 0, 255);
  ledcWrite(pwmChannel, brillo);
}

int calcularBrilloDimming(float lux_ambiente) {
  if (lux_ambiente < UMBRAL_LUZ_MIN) {
    nivel_luz_desc = "Oscuro";
    return 255;
  }
  else if (lux_ambiente > UMBRAL_LUZ_MAX) {
    nivel_luz_desc = "Luminoso";
    return 0;
  }
  else {
    nivel_luz_desc = "Medio";
    return map(lux_ambiente, UMBRAL_LUZ_MIN, UMBRAL_LUZ_MAX, 255, 0);
  }
}

void leerLuzAmbiente() {
  if (!bh1750_disponible) return;
  float temp_lux = lightMeter.readLightLevel();
  if (temp_lux >= 0 && !isnan(temp_lux)) {
    lux_actual = temp_lux;
  }
}

void medirConsumo() {
  if (!ina219_disponible) return;
  
  float temp_shunt_mv = ina219.getShuntVoltage_mV();
  float temp_bus_v = ina219.getBusVoltage_V();
  float temp_c = ina219.getCurrent_mA();
  float temp_p = ina219.getPower_mW();
  
  if (isnan(temp_bus_v) || isnan(temp_c)) return;
  
  voltaje_shunt = temp_shunt_mv;
  voltaje_bus = temp_bus_v;
  corriente = abs(temp_c);
  potencia = temp_p;
  
  unsigned long ahora = millis();
  if (ultimaMedicion > 0 && potencia > 0) {
    float deltaT = (ahora - ultimaMedicion) / 3600000.0;
    energia_acumulada += (potencia / 1000.0) * deltaT;
  }
  ultimaMedicion = ahora;
}

// ========== API REST ENDPOINTS ==========

// GET /api/status - Obtener estado completo
void handleGetStatus() {
  StaticJsonDocument<512> doc;
  
  doc["modo"] = (modoActual == AUTOMATICO) ? "auto" : "manual";
  doc["pir"] = movimientoDetectado;
  doc["brillo_actual"] = brillo_actual;
  doc["brillo_manual"] = brillo_manual;
  
  JsonObject luz = doc.createNestedObject("luz");
  luz["lux"] = lux_actual;
  luz["condicion"] = nivel_luz_desc;
  luz["disponible"] = bh1750_disponible;
  
  JsonObject energia = doc.createNestedObject("energia");
  energia["corriente_ma"] = corriente;
  energia["potencia_mw"] = potencia;
  energia["energia_wh"] = energia_acumulada;
  energia["voltaje_shunt_mv"] = voltaje_shunt;
  energia["disponible"] = ina219_disponible;
  
  doc["uptime_ms"] = millis();
  
  String response;
  serializeJson(doc, response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", response);
}

// POST /api/modo - Cambiar modo (auto/manual)
void handleSetModo() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error && doc.containsKey("modo")) {
      String modo = doc["modo"];
      if (modo == "auto") {
        modoActual = AUTOMATICO;
        Serial.println("✅ Modo: AUTOMÁTICO");
      } else if (modo == "manual") {
        modoActual = MANUAL;
        Serial.println("✅ Modo: MANUAL");
      }
      
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
}

// POST /api/brillo - Controlar brillo manual
void handleSetBrillo() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error && doc.containsKey("brillo")) {
      int brillo = doc["brillo"];
      brillo = constrain(brillo, 0, 255);
      
      if (modoActual == MANUAL) {
        brillo_manual = brillo;
        setBrightness(brillo_manual);
        Serial.printf("🎨 Brillo manual: %d/255\n", brillo_manual);
      }
      
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
}

// OPTIONS para CORS
void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

// GET / - Info básica
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>ESP32 API</title></head><body>";
  html += "<h1>🤖 ESP32 API REST</h1>";
  html += "<h2>Endpoints Disponibles:</h2>";
  html += "<ul>";
  html += "<li><strong>GET /api/status</strong> - Estado completo del sistema</li>";
  html += "<li><strong>POST /api/modo</strong> - Cambiar modo {\"modo\":\"auto|manual\"}</li>";
  html += "<li><strong>POST /api/brillo</strong> - Ajustar brillo {\"brillo\":0-255}</li>";
  html += "</ul>";
  html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
  html += "<p>Uptime: " + String(millis()/1000) + " segundos</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n╔═════════════════════════════════════════════╗");
  Serial.println("║      ESP32 - API REST Únicamente           ║");
  Serial.println("║   Sin Dashboard - Deploy Separado v1.0     ║");
  Serial.println("╚═════════════════════════════════════════════╝\n");
  
  // Configurar hardware
  pinMode(PIN_PIR, INPUT);
  pinMode(PIR_DEBUG_LED, OUTPUT);
  digitalWrite(PIR_DEBUG_LED, LOW);
  
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(MOSFET_PIN, pwmChannel);
  setBrightness(0);
  
  Serial.println("✅ Hardware configurado");
  
  // Calibración PIR (reducida a 10s para desarrollo)
  Serial.println("\n=== Calibrando PIR (10s) ===");
  for(int i = 10; i > 0; i--) {
    digitalWrite(PIR_DEBUG_LED, i % 2);
    if (i % 5 == 0) Serial.printf("%d segundos...\n", i);
    delay(1000);
  }
  digitalWrite(PIR_DEBUG_LED, LOW);
  
  // Configurar I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  delay(100);
  
  // Detectar sensores
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) {
    if (ina219.begin()) {
      ina219_disponible = true;
      ina219.setCalibration_32V_2A();
      Serial.println("✅ INA219 detectado");
    }
  }
  
  Wire.beginTransmission(0x23);
  if (Wire.endTransmission() == 0) {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      bh1750_disponible = true;
      Serial.println("✅ BH1750 detectado");
    }
  }
  
  // Conectar WiFi
  Serial.println("\n📍 Conectando WiFi...");
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi conectado");
    Serial.print("   IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ WiFi no conectado");
  }
  
  // Configurar rutas API
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/modo", HTTP_POST, handleSetModo);
  server.on("/api/modo", HTTP_OPTIONS, handleOptions);
  server.on("/api/brillo", HTTP_POST, handleSetBrillo);
  server.on("/api/brillo", HTTP_OPTIONS, handleOptions);
  
  server.begin();
  
  Serial.println("\n╔═════════════════════════════════════════════╗");
  Serial.println("║         API REST LISTA                      ║");
  Serial.println("╚═════════════════════════════════════════════╝");
  Serial.printf("\n🌐 API URL: http://%s/api/status\n\n", WiFi.localIP().toString().c_str());
}

// ========== LOOP ==========
void loop() {
  server.handleClient();
  
  // Leer PIR
  int estadoPIR = digitalRead(PIN_PIR);
  digitalWrite(PIR_DEBUG_LED, estadoPIR);
  
  if (estadoPIR != estadoPIRAnterior) {
    Serial.printf("PIR: %s\n", estadoPIR ? "DETECTADO" : "SIN MOVIMIENTO");
    estadoPIRAnterior = estadoPIR;
  }
  
  // Leer sensores
  static unsigned long ultimaLectura = 0;
  if (millis() - ultimaLectura > 500) {
    leerLuzAmbiente();
    medirConsumo();
    ultimaLectura = millis();
  }
  
  // Lógica de control automático
  if (modoActual == AUTOMATICO) {
    if (estadoPIR == HIGH) {
      if (!movimientoDetectado) {
        Serial.println("🚶 Movimiento detectado");
        movimientoDetectado = true;
      }
      tiempoUltimoMovimiento = millis();
      brillo_actual = calcularBrilloDimming(lux_actual);
      setBrightness(brillo_actual);
    } else {
      if (movimientoDetectado) {
        if (millis() - tiempoUltimoMovimiento > TIMEOUT_AUTO) {
          Serial.println("💤 Sin movimiento - Apagando");
          movimientoDetectado = false;
          brillo_actual = 0;
          setBrightness(0);
        }
      }
    }
  } else {
    brillo_actual = brillo_manual;
    setBrightness(brillo_manual);
  }
  
  // Reporte periódico
  static unsigned long ultimoReporte = 0;
  if (millis() - ultimoReporte > 5000) {
    Serial.println("\n┌──────────────────────────────────────────┐");
    Serial.printf("│ PIR: %s │ Luz: %.1f lx │ Brillo: %d/255 │\n", 
                  movimientoDetectado ? "✓" : "✗", lux_actual, brillo_actual);
    Serial.printf("│ I: %.1fmA │ P: %.1fmW │ Modo: %s │\n", 
                  corriente, potencia, modoActual == AUTOMATICO ? "AUTO" : "MANUAL");
    Serial.println("└──────────────────────────────────────────┘");
    ultimoReporte = millis();
  }
  
  delay(50);
}