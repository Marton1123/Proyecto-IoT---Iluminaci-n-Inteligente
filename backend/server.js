// ============================================
// Backend Node.js + Express + WebSocket
// ============================================

const express = require('express');
const cors = require('cors');
const axios = require('axios');
const WebSocket = require('ws');

const app = express();
const PORT = 3000;

// Configuración del ESP32
const ESP32_IP = '192.168.1.100'; // ⚠️ CAMBIAR POR LA IP DE TU ESP32
const ESP32_API = `http://${ESP32_IP}`;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public')); // Para servir el frontend

// ============================================
// RUTAS API (Proxy al ESP32)
// ============================================

// GET /api/status - Obtener estado del ESP32
app.get('/api/status', async (req, res) => {
  try {
    const response = await axios.get(`${ESP32_API}/api/status`, {
      timeout: 5000
    });
    res.json(response.data);
  } catch (error) {
    console.error('❌ Error conectando con ESP32:', error.message);
    res.status(503).json({ 
      error: 'ESP32 no disponible',
      message: error.message 
    });
  }
});

// POST /api/modo - Cambiar modo
app.post('/api/modo', async (req, res) => {
  try {
    const { modo } = req.body;
    
    if (!modo || !['auto', 'manual'].includes(modo)) {
      return res.status(400).json({ error: 'Modo inválido. Usar: auto o manual' });
    }
    
    const response = await axios.post(
      `${ESP32_API}/api/modo`,
      { modo },
      { timeout: 5000 }
    );
    
    res.json(response.data);
    
    // Notificar a todos los clientes WebSocket
    broadcastToClients({ type: 'modo_changed', modo });
    
  } catch (error) {
    console.error('❌ Error cambiando modo:', error.message);
    res.status(503).json({ error: 'Error al comunicar con ESP32' });
  }
});

// POST /api/brillo - Controlar brillo
app.post('/api/brillo', async (req, res) => {
  try {
    const { brillo } = req.body;
    
    if (brillo === undefined || brillo < 0 || brillo > 255) {
      return res.status(400).json({ error: 'Brillo debe estar entre 0 y 255' });
    }
    
    const response = await axios.post(
      `${ESP32_API}/api/brillo`,
      { brillo },
      { timeout: 5000 }
    );
    
    res.json(response.data);
    
    // Notificar a todos los clientes WebSocket
    broadcastToClients({ type: 'brillo_changed', brillo });
    
  } catch (error) {
    console.error('❌ Error ajustando brillo:', error.message);
    res.status(503).json({ error: 'Error al comunicar con ESP32' });
  }
});

// GET /api/ping - Verificar conexión con ESP32
app.get('/api/ping', async (req, res) => {
  try {
    const start = Date.now();
    await axios.get(`${ESP32_API}/api/status`, { timeout: 3000 });
    const latency = Date.now() - start;
    
    res.json({ 
      status: 'online', 
      latency_ms: latency,
      esp32_ip: ESP32_IP 
    });
  } catch (error) {
    res.json({ 
      status: 'offline', 
      error: error.message,
      esp32_ip: ESP32_IP 
    });
  }
});

// ============================================
// WEBSOCKET SERVER (Tiempo Real)
// ============================================

const server = app.listen(PORT, () => {
  console.log('\n╔═══════════════════════════════════════════╗');
  console.log('║   🚀 Backend Node.js Iniciado             ║');
  console.log('╚═══════════════════════════════════════════╝');
  console.log(`\n📡 Servidor HTTP: http://localhost:${PORT}`);
  console.log(`🌐 Frontend: http://localhost:${PORT}/index.html`);
  console.log(`🔌 WebSocket: ws://localhost:${PORT}`);
  console.log(`🤖 ESP32 API: ${ESP32_API}\n`);
});

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
  console.log('✅ Cliente WebSocket conectado');
  
  ws.on('message', (message) => {
    console.log('📩 Mensaje recibido:', message.toString());
  });
  
  ws.on('close', () => {
    console.log('❌ Cliente WebSocket desconectado');
  });
  
  // Enviar estado inicial
  ws.send(JSON.stringify({ type: 'connected', message: 'Conectado al servidor' }));
});

// Función para enviar datos a todos los clientes conectados
function broadcastToClients(data) {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(data));
    }
  });
}

// ============================================
// POLLING AL ESP32 (cada 2 segundos)
// ============================================

setInterval(async () => {
  try {
    const response = await axios.get(`${ESP32_API}/api/status`, { timeout: 3000 });
    
    // Enviar actualización a todos los clientes conectados
    broadcastToClients({
      type: 'status_update',
      data: response.data,
      timestamp: Date.now()
    });
    
  } catch (error) {
    // Si el ESP32 no responde, notificar a los clientes
    broadcastToClients({
      type: 'esp32_offline',
      message: 'ESP32 no disponible',
      timestamp: Date.now()
    });
  }
}, 2000);

// Manejo de errores
process.on('uncaughtException', (err) => {
  console.error('❌ Error no capturado:', err);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('❌ Promesa rechazada:', reason);
});