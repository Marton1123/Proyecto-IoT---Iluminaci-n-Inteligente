#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_INA219.h>

// ==========================================================
// ----- SELECCIÓN DE MODO DE PRUEBA -----
// ==========================================================
// true = Modo "Inteligente" (PIR + Luz + Dimming)
// false = Modo "Tonto" (PIR + ON/OFF al 100%)
const bool MODO_PRUEBA_AUTO = false; 
// ==========================================================


// --- Pines y Sensores ---
#define MOSFET_PIN 25
#define PIR_PIN 27
BH1750 lightMeter;
Adafruit_INA219 ina219;

// --- Configs de PWM y Luz ---
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;
const float UMBRAL_LUZ_MIN = 50.0; 
const float UMBRAL_LUZ_MAX = 400.0;
int brillo_actual = 0;

// --- Variables para el Benchmark ---
unsigned long tiempoPruebaInicio = 0;
unsigned long ultimoTiempoLoop = 0; // << DECLARACIÓN ORIGINAL (con 'e')
unsigned long ultimaImpresion = 0;
double energiaTotalAcumulada_mWs = 0; // miliWatt-segundos

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(PIR_PIN, INPUT);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  ina219.begin();
  ina219.setCalibration_32V_2A();
  
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(MOSFET_PIN, pwmChannel);

  Serial.println("\n\nIniciando Benchmark de Consumo Energético");
  if (MODO_PRUEBA_AUTO) {
    Serial.println("MODO: 'Inteligente' (Dimming Automático) ACTIVADO");
  } else {
    Serial.println("MODO: 'Tonto' (ON/OFF al 100%) ACTIVADO");
  }

  // Reiniciar contadores
  tiempoPruebaInicio = millis();
  ultimoTiempoLoop = millis(); // << CORREGIDO (con 'e')
  ultimaImpresion = millis();
  
  Serial.println("-----------------------------------------------------------------");
  Serial.println("Tiempo (seg) \t| Luz (lx) \t| Potencia (mW) \t| Energía Total (mWs)");
  Serial.println("-----------------------------------------------------------------");
}

void loop() {
  // --- Cálculo del tiempo transcurrido ---
  unsigned long ahora = millis();
  double tiempoTranscurrido_seg = (ahora - ultimoTiempoLoop) / 1000.0; // << CORREGIDO (con 'e')
  ultimoTiempoLoop = ahora; // << CORREGIDO (con 'e')

  // --- Lógica de Control ---
  int pirState = digitalRead(PIR_PIN);
  brillo_actual = 0; // Por defecto, apagado

  if (pirState == HIGH) { // Si hay presencia...
    if (MODO_PRUEBA_AUTO) {
      // --- Lógica MODO INTELIGENTE ---
      float lux_ambiente = lightMeter.readLightLevel(); // Lee la luz para la lógica
      if (lux_ambiente < UMBRAL_LUZ_MIN) brillo_actual = 255;
      else if (lux_ambiente > UMBRAL_LUZ_MAX) brillo_actual = 0;
      else brillo_actual = map(lux_ambiente, UMBRAL_LUZ_MIN, UMBRAL_LUZ_MAX, 255, 0);
    } 
    else {
      // --- Lógica MODO TONTO ---
      brillo_actual = 255; // Encendido al 100%
    }
  }
  
  // Aplicar brillo
  ledcWrite(pwmChannel, brillo_actual);

  // --- Medición de Consumo ---
  float power_mW = ina219.getPower_mW();
  if (power_mW < 0) power_mW = 0;

  // Acumular la energía
  energiaTotalAcumulada_mWs += (power_mW * tiempoTranscurrido_seg);

  // --- Reporte cada 2 segundos ---
  if (ahora - ultimaImpresion > 2000) {
    ultimaImpresion = ahora;
    unsigned long tiempoTotalPrueba_seg = (ahora - tiempoPruebaInicio) / 1000;
    
    float lux_reporte = lightMeter.readLightLevel();

    Serial.print(tiempoTotalPrueba_seg);
    Serial.print(" seg \t\t| ");
    Serial.print(lux_reporte);
    Serial.print(" lx \t| ");
    Serial.print(power_mW);
    Serial.print(" mW \t\t| ");
    Serial.print(energiaTotalAcumulada_mWs, 2); 
    Serial.println(" mWs");
  }
}