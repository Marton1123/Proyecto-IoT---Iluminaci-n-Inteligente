#include <WiFi.h>

// ==========================================================
// ----- CONFIGURACIÓN WIFI (Tu Info) -----
// ==========================================================
const char* ssid = "WiFi_Mesh-691458";
const char* password = "";
// ==========================================================

// --- Servidor Web en el puerto 80 ---
WiFiServer server(80);

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
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // --- Iniciar el Servidor Web ---
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
  Serial.println("------------------------------------");
}

// ==========================================================
// ----- FUNCIÓN LOOP() (Aquí está la corrección) -----
// ==========================================================
void loop() {
  WiFiClient client = server.available(); // Esperar a que un cliente se conecte

  if (client) { // Si un cliente se conecta...
    Serial.println("\n[Nuevo cliente conectado]");
    String currentLine = "";  // String para guardar la línea actual
    String requestLine = "";  // String para guardar la PRIMERA línea (el comando)
    bool isFirstLine = true;  // Bandera para saber si es la primera línea

    while (client.connected()) {
      if (client.available()) {
        char c = client.read(); // Leer un byte
        Serial.write(c); // Imprimir la petición del cliente en el Monitor Serie
        
        if (c == '\n') { // Si es el fin de una línea
          
          if (isFirstLine) {
            requestLine = currentLine; // Guardamos la primera línea (ej. "GET /on HTTP/1.1")
            isFirstLine = false;
          }
          
          // Si la línea está vacía, es el fin de la petición HTTP
          if (currentLine.length() == 0) {
            
            // --- Respuesta HTTP ---
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close"); // Le decimos al navegador que cerraremos la conexión
            client.println();
            
            // --- Página Web (HTML) ---
            client.println("<html><body>");
            client.println("<h1>Control de Luz (ESP32)</h1>");
            
            // --- Lógica de Control (AHORA COMPRUEBA 'requestLine') ---
            if (requestLine.indexOf("GET /on") >= 0) {
              Serial.println(">> Acción: Encendiendo luz (100%)");
              ledcWrite(pwmChannel, 255);
              client.println("<h2>Estado: LUZ ENCENDIDA</h2>");
            } else if (requestLine.indexOf("GET /off") >= 0) {
              Serial.println(">> Acción: Apagando luz (0%)");
              ledcWrite(pwmChannel, 0);
              client.println("<h2>Estado: LUZ APAGADA</h2>");
            } else if (requestLine.indexOf("GET /favicon.ico") >= 0) {
              // Ignorar la petición del ícono
              Serial.println(">> Acción: Ignorando favicon.ico");
            } else {
              client.println("<h2>Comando no reconocido.</h2>");
            }
            
            client.println("<p><a href='/on'><button>ENCENDER</button></a></p>");
            client.println("<p><a href='/off'><button>APAGAR</button></a></p>");
            client.println("</body></html>");
            
            // Fin de la respuesta
            break; 
          } else {
            // Si no es una línea vacía, limpiarla para leer la siguiente
            currentLine = "";
          }
        } else if (c != '\r') {
          // Añadir el caracter al string de la línea actual
          currentLine += c; 
        }
      }
    }
    // Cerrar la conexión
    client.stop();
    Serial.println("[Cliente desconectado]");
  }
}