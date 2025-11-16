#include <WiFi.h>
#include <PubSubClient.h>

// --- Configuración WiFi (Tus credenciales) ---
const char* ssid = "WiFi_Mesh-691458";
const char* password = "";

// --- Configuración MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// ¡Tu tema único!
const char* mqtt_topic = "proyecto/mict_esp32/luzInteligente"; 
const char* mqtt_topic_ping = "proyecto/mict_esp32/ping"; // Un tema separado para el ping

WiFiClient espClient;
PubSubClient client(espClient);

// --- Variables para medir latencia ---
unsigned long tiempoEnvioPing = 0;
bool esperandoPing = false;

void setup() {
  Serial.begin(115200);
  
  // Conectar a WiFi
  Serial.println("\nConectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n¡WiFi conectado!");
  Serial.println(WiFi.localIP());

  // Conectar a MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Asignar la función que recibe mensajes
}

// --- Esta función se llama CADA VEZ que llega un mensaje ---
void callback(char* topic, byte* payload, unsigned int length) {
  // Convertir el tema a String para compararlo
  String topicStr = String(topic);

  // --- Revisar si es nuestro PING de vuelta ---
  if (topicStr == mqtt_topic_ping && esperandoPing) {
    unsigned long tiempoLlegada = millis();
    unsigned long latencia = tiempoLlegada - tiempoEnvioPing;
    
    Serial.println("-----------------------");
    Serial.print("¡PING RECIBIDO! ");
    Serial.print("Latencia (Nube): ");
    Serial.print(latencia);
    Serial.println(" ms");
    Serial.println("-----------------------");
    
    esperandoPing = false; // Dejamos de esperar, listos para el próximo ping
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Dale un ID de cliente único
    if (client.connect("esp32ClienteUnico_Marti_Ping")) { 
      Serial.println("¡Conectado!");
      // Suscribirse a AMBOS temas
      client.subscribe(mqtt_topic);
      client.subscribe(mqtt_topic_ping);
      Serial.println("Suscrito a los temas de luz y ping.");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" ...reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Mantener la conexión viva

  // --- Enviar un PING cada 5 segundos ---
  // (Solo si no estamos ya esperando una respuesta)
  if (!esperandoPing && (millis() - tiempoEnvioPing > 5000)) {
    Serial.println("\nEnviando PING a la nube...");
    tiempoEnvioPing = millis(); // Registrar la hora de envío
    esperandoPing = true;
    client.publish(mqtt_topic_ping, "ping"); // Enviar el ping
  }
}