#include <WiFi.h>
#include <PubSubClient.h> // La nueva librería

// ==========================================================
// ----- CONFIGURACIÓN WIFI -----
const char* ssid = "WiFi_Mesh-691458";
const char* password = "";

// ----- CONFIGURACIÓN MQTT -----
// Usaremos un broker de prueba público y gratuito
const char* mqtt_server = "broker.hivemq.com"; 
const int mqtt_port = 1883;

// ¡¡CAMBIAR ESTO por algo ÚNICO!! (ej. "proyecto/marti/luz")
const char* mqtt_topic = "proyecto/mict_esp32/luzInteligente"; 
// ==========================================================

// --- Clientes ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- PIN DEL MOSFET ---
#define MOSFET_PIN 25
const int pwmChannel = 0;

void setup() {
  Serial.begin(115200);

  // --- Setup del MOSFET ---
  const int pwmFreq = 5000;
  const int pwmResolution = 8;
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(MOSFET_PIN, pwmChannel);
  ledcWrite(pwmChannel, 0); // Empezar con la luz apagada

  // --- Conectar a WiFi ---
  Serial.println("\nConectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡WiFi conectado!");
  Serial.println(WiFi.localIP());

  // --- Conectar a MQTT ---
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Asignar la función que recibe mensajes
  Serial.println("Conectando al Broker MQTT...");
  reconnect();
}

// --- Esta función se llama CADA VEZ que llega un mensaje ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("-----------------------");
  Serial.print("¡Mensaje recibido! [");
  Serial.print(topic);
  Serial.print("] ");

  // Convertir el mensaje (payload) a un String
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // --- Lógica de ON/OFF ---
  if (message == "ON") {
    Serial.println(">> Acción: Encendiendo luz (100%)");
    ledcWrite(pwmChannel, 255); // MOSFET al 100%
  } else if (message == "OFF") {
    Serial.println(">> Acción: Apagando luz (0%)");
    ledcWrite(pwmChannel, 0); // MOSFET al 0%
  }
}

// --- Función para (re)conectarse al Broker ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Intentar conectar
    if (client.connect("esp32ClienteUnico_Marti")) { // <-- Dale un ID de cliente único
      Serial.println("¡Conectado!");
      // Suscribirse al tema
      client.subscribe(mqtt_topic);
      Serial.print("Suscrito al tema: ");
      Serial.println(mqtt_topic);
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" ...reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void loop() {
  // Esta función mantiene la conexión MQTT viva
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 
}