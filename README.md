# Proyecto IoT Iluminación Inteligente

Sistema distribuido con ESP32 (sensores/actuadores), Backend Node.js y Frontend web.

## Arquitectura del Sistema

```
┌──────────────┐     WebSocket/HTTP     ┌──────────────┐
│   FRONTEND   │ ←────────────────────→ │   BACKEND    │
│  (Browser)   │    localhost:3000      │  (Node.js)   │
└──────────────┘                        └──────────────┘
                                               ↕
                                        HTTP REST API
                                               ↕
                                        ┌──────────────┐
                                        │    ESP32     │
                                        │  192.168.x.x │
                                        └──────────────┘
```

---

## PARTE 1: Configurar el ESP32

#### 1.1 Hardware Requerido
- ESP32 DevKit
- Sensor PIR (pin D27)
- MOSFET para LED (pin D25)
- LED de prueba/carga
- Sensor BH1750 (I2C: SDA=21, SCL=22)
- Sensor INA219 (I2C: 0x40)


#### 1.2 Librerías Arduino
Instala estas librerías desde el Library Manager:

- WiFi (incluida en ESP32)
- WebServer (incluida en ESP32)
- Wire (incluida)
- Adafruit_INA219
- BH1750 (by Christopher Laws)
- ArduinoJson (v6.x)


#### 1.3 Configuración WiFi
En el código ESP32, modifica estas líneas:
```
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";
```

#### 1.4 Subir el Código

1. Abre Arduino IDE
2. Selecciona placa: ESP32 Dev Module
3. Sube el código ESP32_API_Only.ino
4. Abre el Monitor Serial (115200 baud)
5. Anota la IP que muestra (ejemplo: 192.168.1.100)


#### 1.5 Verificar API
Abre el navegador y verifica:

```
http://192.168.1.100/
http://192.168.1.100/api/status
```

---

## PARTE 2: Configurar el Backend (PC)

#### 2.1 Prerequisitos
Instala Node.js (v16 o superior):

- Descarga desde: https://nodejs.org/

#### 2.2 Estructura de Carpetas
Crea esta estructura en tu PC:
```
iluminacion-backend/
├── server.js
├── package.json
└── public/
    └── index.html
```

### 2.3 Instalación
Abre terminal/cmd en la carpeta del proyecto:
```
# Instalar dependencias
npm install

# O instalar manualmente
npm install express cors axios ws
```

#### 2.4 Configurar IP del ESP32
Edita server.js línea 13:
```
const ESP32_IP = '192.168.1.100';
```

#### 2.5 Iniciar el Servidor

```
npm start
```

Deberías ver:
```
╔═══════════════════════════════════════════╗
║   🚀 Backend Node.js Iniciado             ║
╚═══════════════════════════════════════════╝

📡 Servidor HTTP: http://localhost:3000
🌐 Frontend: http://localhost:3000/index.html
🔌 WebSocket: ws://localhost:3000
🤖 ESP32 API: http://192.168.1.100
```

#### 2.6 Verificar Backend
Abre navegador:

```
http://localhost:3000/api/ping
http://localhost:3000/api/status
```

---

## PARTE 3: Acceder al Dashboard

#### 3.1 Abrir Frontend
Navega a:
```
http://localhost:3000/index.html
```

3.2 Indicadores de Conexión

- 🟢 Conectado: Todo funciona correctamente
- 🔴 Desconectado: ESP32 no responde
- 🔄 Conectando: Intentando reconectar

---

#### API REST Disponible
**Endpoints del ESP32**

- GET `/api/status`
Obtiene el estado completo del sistema.
Respuesta:
```
{
  "modo": "auto",
  "pir": true,
  "brillo_actual": 255,
  "brillo_manual": 128,
  "luz": {
    "lux": 45.3,
    "condicion": "Oscuro",
    "disponible": true
  },
  "energia": {
    "corriente_ma": 125.4,
    "potencia_mw": 312.5,
    "energia_wh": 0.156,
    "voltaje_shunt_mv": 2.45,
    "disponible": true
  },
  "uptime_ms": 45000
}
```


- POST `/api/modo`
Cambia el modo de operación.
Body:
```
{
  "modo": "auto"  // o "manual"
}
```

- POST `/api/brillo`
Ajusta el brillo en modo manual.
Body:
```
{
  "brillo": 200  // 0-255
}
```

---

## Posibles problemas:
#### ESP32 no se conecta al WiFi

1. Verifica SSID y contraseña
2. Revisa que el WiFi sea 2.4GHz (no 5GHz)
3. Acércate al router

####  Backend no se conecta al ESP32

1. Verifica que ambos estén en la misma red
2. Haz ping al ESP32: `ping 192.168.1.100`
3. Verifica que la IP en `server.js` sea correcta
4. Desactiva firewall temporalmente

#### Frontend no muestra datos

1. Abre la consola del navegador (F12)
2. Verifica errores de conexión
3. Asegúrate que el backend esté corriendo
4. Recarga la página (Ctrl+F5)

#### WebSocket se desconecta

1. Normal, el sistema reintenta automáticamente cada 3s
2. Verifica que el puerto 3000 no esté bloqueado

---

## Soporte
Si tienes problemas:

1. Revisa el Monitor Serial del ESP32
2. Verifica logs del backend (terminal)
3. Abre consola del navegador (F12)

---

### Licencia
MIT License - Úsalo libremente para tus proyectos 🎉