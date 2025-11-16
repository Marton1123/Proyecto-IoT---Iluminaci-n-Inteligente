// ============================================
// Backend Node.js + Express + WebSocket + MongoDB
// ============================================
require('dotenv').config();
const path = require('path');
const express = require('express');
const cors = require('cors');
const axios = require('axios');
const WebSocket = require('ws');
const mongoose = require('mongoose');
const jwt = require('jsonwebtoken');
const { body, validationResult } = require('express-validator');

// Importar modelos y middleware
const User = require('./models/User');
const { authenticate, requireAdmin } = require('./middleware/auth');

const app = express();
const PORT = process.env.PORT || 3000;

// Configuración del ESP32
const ESP32_IP = process.env.ESP32_IP || '192.168.1.105';
const ESP32_API = `http://${ESP32_IP}`;

// Middleware
app.use(cors());
app.use(express.json());

// Servir archivos estáticos - probar varias rutas posibles
const possiblePaths = [
  path.join(__dirname, '..', 'frontend', 'public'),
  path.join(__dirname, 'public'),
  path.join(__dirname, '..', 'public'),
  path.join(__dirname, 'frontend', 'public')
];

// Intentar encontrar el directorio correcto
let staticPath = possiblePaths[0];
const fs = require('fs');
for (const testPath of possiblePaths) {
  if (fs.existsSync(testPath)) {
    staticPath = testPath;
    console.log(`📁 Sirviendo archivos estáticos desde: ${staticPath}`);
    break;
  }
}

app.use(express.static(staticPath));

// ============================================
// CONEXIÓN A MONGODB
// ============================================
mongoose.connect(process.env.MONGODB_URI, {
  useNewUrlParser: true,
  useUnifiedTopology: true
})
.then(() => {
  console.log('✅ Conectado a MongoDB');
})
.catch((error) => {
  console.error('❌ Error conectando a MongoDB:', error);
  process.exit(1);
});

// ============================================
// RUTAS DE AUTENTICACIÓN
// ============================================

// POST /api/auth/register - Registrar nuevo usuario
app.post('/api/auth/register', [
  body('username').trim().isLength({ min: 3, max: 30 }).withMessage('Usuario debe tener entre 3 y 30 caracteres'),
  body('email').isEmail().normalizeEmail().withMessage('Email inválido'),
  body('password').isLength({ min: 6 }).withMessage('Contraseña debe tener al menos 6 caracteres')
], async (req, res) => {
  try {
    // Validar datos
    const errors = validationResult(req);
    if (!errors.isEmpty()) {
      return res.status(400).json({ errors: errors.array() });
    }

    const { username, email, password } = req.body;

    // Verificar si el usuario ya existe
    const existingUser = await User.findOne({ $or: [{ username }, { email }] });
    if (existingUser) {
      return res.status(400).json({ 
        error: 'Usuario ya existe',
        message: 'El nombre de usuario o email ya está registrado' 
      });
    }

    // Crear nuevo usuario
    const user = new User({
      username,
      email,
      password,
      role: 'user'
    });

    await user.save();

    // Generar token
    const token = jwt.sign(
      { userId: user._id, username: user.username, role: user.role },
      process.env.JWT_SECRET,
      { expiresIn: '7d' }
    );

    res.status(201).json({
      message: 'Usuario registrado exitosamente',
      token,
      user: {
        id: user._id,
        username: user.username,
        email: user.email,
        role: user.role
      }
    });

  } catch (error) {
    console.error('❌ Error en registro:', error);
    res.status(500).json({ 
      error: 'Error en el servidor',
      message: error.message 
    });
  }
});

// POST /api/auth/login - Iniciar sesión
app.post('/api/auth/login', [
  body('username').trim().notEmpty().withMessage('Usuario requerido'),
  body('password').notEmpty().withMessage('Contraseña requerida')
], async (req, res) => {
  try {
    // Validar datos
    const errors = validationResult(req);
    if (!errors.isEmpty()) {
      return res.status(400).json({ errors: errors.array() });
    }

    const { username, password } = req.body;

    // Buscar usuario (por username o email)
    const user = await User.findOne({
      $or: [{ username }, { email: username }],
      isActive: true
    });

    if (!user) {
      return res.status(401).json({ 
        error: 'Credenciales inválidas',
        message: 'Usuario o contraseña incorrectos' 
      });
    }

    // Verificar contraseña
    const isMatch = await user.comparePassword(password);
    if (!isMatch) {
      return res.status(401).json({ 
        error: 'Credenciales inválidas',
        message: 'Usuario o contraseña incorrectos' 
      });
    }

    // Actualizar último login
    user.lastLogin = new Date();
    await user.save();

    // Generar token
    const token = jwt.sign(
      { userId: user._id, username: user.username, role: user.role },
      process.env.JWT_SECRET,
      { expiresIn: '7d' }
    );

    res.json({
      message: 'Login exitoso',
      token,
      user: {
        id: user._id,
        username: user.username,
        email: user.email,
        role: user.role,
        lastLogin: user.lastLogin
      }
    });

  } catch (error) {
    console.error('❌ Error en login:', error);
    res.status(500).json({ 
      error: 'Error en el servidor',
      message: error.message 
    });
  }
});

// GET /api/auth/me - Obtener información del usuario actual
app.get('/api/auth/me', authenticate, async (req, res) => {
  res.json({
    user: req.user
  });
});

// POST /api/auth/logout - Cerrar sesión (solo para registro)
app.post('/api/auth/logout', authenticate, async (req, res) => {
  res.json({ message: 'Sesión cerrada exitosamente' });
});

// ============================================
// RUTAS API (Protegidas con autenticación)
// ============================================

// GET /api/status - Obtener estado del ESP32
app.get('/api/status', authenticate, async (req, res) => {
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
app.post('/api/modo', authenticate, async (req, res) => {
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
    broadcastToClients({ 
      type: 'modo_changed', 
      modo,
      user: req.user.username 
    });
    
  } catch (error) {
    console.error('❌ Error cambiando modo:', error.message);
    res.status(503).json({ error: 'Error al comunicar con ESP32' });
  }
});

// POST /api/brillo - Controlar brillo
app.post('/api/brillo', authenticate, async (req, res) => {
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
    broadcastToClients({ 
      type: 'brillo_changed', 
      brillo,
      user: req.user.username 
    });
    
  } catch (error) {
    console.error('❌ Error ajustando brillo:', error.message);
    res.status(503).json({ error: 'Error al comunicar con ESP32' });
  }
});

// GET /api/ping - Verificar conexión con ESP32
app.get('/api/ping', authenticate, async (req, res) => {
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
// RUTAS DE ADMINISTRACIÓN (Solo Admin)
// ============================================

// GET /api/admin/users - Listar usuarios
app.get('/api/admin/users', authenticate, requireAdmin, async (req, res) => {
  try {
    const users = await User.find().select('-password');
    res.json({ users });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// DELETE /api/admin/users/:id - Eliminar usuario
app.delete('/api/admin/users/:id', authenticate, requireAdmin, async (req, res) => {
  try {
    const user = await User.findByIdAndDelete(req.params.id);
    if (!user) {
      return res.status(404).json({ error: 'Usuario no encontrado' });
    }
    res.json({ message: 'Usuario eliminado exitosamente' });
  } catch (error) {
    res.status(500).json({ error: error.message });
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
  console.log(`🤖 ESP32 API: ${ESP32_API}`);
  console.log(`🗄️  MongoDB: ${process.env.MONGODB_URI}\n`);
});

const wss = new WebSocket.Server({ server });

// Map para guardar usuarios autenticados por WebSocket
const authenticatedClients = new Map();

wss.on('connection', (ws, req) => {
  console.log('🔌 Nueva conexión WebSocket');
  
  // Manejar autenticación por WebSocket
  ws.on('message', async (message) => {
    try {
      const data = JSON.parse(message.toString());
      
      // Autenticación inicial
      if (data.type === 'auth') {
        try {
          const decoded = jwt.verify(data.token, process.env.JWT_SECRET);
          const user = await User.findById(decoded.userId).select('-password');
          
          if (user && user.isActive) {
            authenticatedClients.set(ws, user);
            ws.send(JSON.stringify({ 
              type: 'auth_success', 
              message: 'Autenticado correctamente',
              user: { username: user.username, role: user.role }
            }));
            console.log(`✅ Usuario autenticado: ${user.username}`);
          } else {
            ws.send(JSON.stringify({ type: 'auth_error', message: 'Usuario inválido' }));
            ws.close();
          }
        } catch (error) {
          ws.send(JSON.stringify({ type: 'auth_error', message: 'Token inválido' }));
          ws.close();
        }
      }
    } catch (error) {
      console.error('❌ Error procesando mensaje:', error);
    }
  });
  
  ws.on('close', () => {
    const user = authenticatedClients.get(ws);
    if (user) {
      console.log(`❌ Usuario desconectado: ${user.username}`);
      authenticatedClients.delete(ws);
    }
  });
});

// Función para enviar datos solo a clientes autenticados
function broadcastToClients(data) {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN && authenticatedClients.has(client)) {
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
    
    // Enviar actualización solo a clientes autenticados
    broadcastToClients({
      type: 'status_update',
      data: response.data,
      timestamp: Date.now()
    });
    
  } catch (error) {
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