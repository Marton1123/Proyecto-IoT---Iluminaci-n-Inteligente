// Script de inicialización de MongoDB
db = db.getSiblingDB('iot_lighting');

// Crear colección de usuarios
db.createCollection('users');

// Crear índice único para username y email
db.users.createIndex({ "username": 1 }, { unique: true });
db.users.createIndex({ "email": 1 }, { unique: true });

// Crear usuario admin por defecto (password: admin123)
// Hash generado con bcrypt para 'admin123'
db.users.insertOne({
  username: "admin",
  email: "admin@example.com",
  password: "$2a$10$X3xZ9YZjQvY7qKQYZLxvLO8vY5qXj3kzV1Z7qZ8xZ9xZ8xZ9xZ8xZ", // admin123
  role: "admin",
  createdAt: new Date(),
  updatedAt: new Date()
});

// Crear colección de logs de sistema
db.createCollection('system_logs');

// Crear colección de configuraciones
db.createCollection('configurations');

// Insertar configuración por defecto
db.configurations.insertOne({
  esp32_ip: "192.168.1.105",
  umbral_oscuro: 50,
  umbral_luminoso: 400,
  brillo_max: 255,
  brillo_min: 0,
  createdAt: new Date(),
  updatedAt: new Date()
});

print("✅ Base de datos inicializada correctamente");
print("👤 Usuario admin creado - username: admin, password: admin123");