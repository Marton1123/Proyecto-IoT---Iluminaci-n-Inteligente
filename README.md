# 🏠 Sistema de Iluminación Inteligente IoT

Sistema completo de iluminación inteligente con ESP32, sensores, y control web con autenticación.

## 🏗️ Estructura del Proyecto

```
proyecto/
├── docker-compose.yml
├── mongo-init.js
├── backend/
│   ├── Dockerfile
│   ├── package.json
│   ├── server.js
│   ├── .env
│   ├── .dockerignore
│   ├── models/
│   │   └── User.js
│   └── middleware/
│       └── auth.js
└── frontend/
    └── public/
        ├── login.html
        └── index.html
```

## 🚀 Instalación Rápida con Docker

### Prerrequisitos

- Docker y Docker Compose instalados
- Git (opcional)

### Pasos de Instalación

1. **Clonar o crear la estructura del proyecto:**

```bash
mkdir iot-lighting-system
cd iot-lighting-system
```

2. **Configurar variables de entorno:**

```bash
cd backend
cp .env.example .env
```

Edita `.env` y cambia:
- `ESP32_IP` con la IP de tu ESP32
- `JWT_SECRET` con una clave secreta segura
- `SESSION_SECRET` con otra clave secreta

3. **Levantar todos los servicios:**

```bash
# Desde la raíz del proyecto
docker-compose up -d
```

Esto levantará:
- **MongoDB** en puerto 27017
- **Backend Node.js** en puerto 3000
- **Mongo Express** (admin UI) en puerto 8081

4. **Verificar que todo esté corriendo:**

```bash
docker-compose ps
```

Deberías ver 3 contenedores corriendo:
- `iot_mongodb`
- `iot_backend`
- `iot_mongo_express`

5. **Acceder a la aplicación:**

- **Frontend:** http://localhost:3000/login.html
- **Mongo Express:** http://localhost:8081 (usuario: admin, password: admin)

## 👤 Usuarios por Defecto

El sistema crea automáticamente un usuario administrador:

- **Usuario:** `admin`
- **Contraseña:** `admin123`
- **Email:** `admin@example.com`

⚠️ **Importante:** Cambia esta contraseña en producción.

## 🔧 Comandos Útiles

### Ver logs en tiempo real:
```bash
docker-compose logs -f backend
```

### Detener todos los servicios:
```bash
docker-compose down
```

### Detener y eliminar volúmenes (borra la BD):
```bash
docker-compose down -v
```

### Reiniciar un servicio específico:
```bash
docker-compose restart backend
```

### Reconstruir imagen del backend:
```bash
docker-compose build backend
docker-compose up -d backend
```

### Ejecutar comandos dentro del contenedor:
```bash
docker exec -it iot_backend sh
```

### Ver logs de MongoDB:
```bash
docker-compose logs mongodb
```

## 🔐 Características de Seguridad

- ✅ Autenticación JWT con tokens de 7 días
- ✅ Contraseñas hasheadas con bcrypt
- ✅ Validación de datos con express-validator
- ✅ Middleware de autenticación para proteger rutas
- ✅ Roles de usuario (user/admin)
- ✅ WebSocket autenticado

## 📡 API Endpoints

### Autenticación (Públicos)

- `POST /api/auth/register` - Registrar nuevo usuario
- `POST /api/auth/login` - Iniciar sesión

### Protegidos (Requieren token)

- `GET /api/auth/me` - Info del usuario actual
- `POST /api/auth/logout` - Cerrar sesión
- `GET /api/status` - Estado del ESP32
- `POST /api/modo` - Cambiar modo (auto/manual)
- `POST /api/brillo` - Ajustar brillo
- `GET /api/ping` - Ping al ESP32

### Admin (Requieren rol admin)

- `GET /api/admin/users` - Listar usuarios
- `DELETE /api/admin/users/:id` - Eliminar usuario

## 🌐 WebSocket

El WebSocket requiere autenticación. Al conectar, envía:

```javascript
{
  "type": "auth",
  "token": "tu_jwt_token_aqui"
}
```

## 🛠️ Desarrollo Local (sin Docker)

Si prefieres desarrollo sin Docker:

```bash
# Instalar MongoDB localmente
# En macOS: brew install mongodb-community
# En Ubuntu: sudo apt install mongodb

# Iniciar MongoDB
mongod --dbpath /path/to/data

# Instalar dependencias
cd backend
npm install

# Copiar .env
cp .env.example .env

# Editar .env y cambiar MONGODB_URI a:
# MONGODB_URI=mongodb://localhost:27017/iot_lighting

# Iniciar servidor
npm start
```

## 🔍 Monitoreo

### Mongo Express (UI de administración)
Accede a http://localhost:8081 para:
- Ver la base de datos
- Gestionar usuarios
- Ver logs del sistema
- Exportar/importar datos

Usuario: `admin`  
Password: `admin`

## 🐛 Solución de Problemas

### El backend no se conecta a MongoDB:

```bash
# Verificar que MongoDB esté corriendo
docker-compose logs mongodb

# Reiniciar MongoDB
docker-compose restart mongodb
```

### El ESP32 no responde:

1. Verifica que la IP en `.env` sea correcta
2. Asegúrate de que el ESP32 y el servidor estén en la misma red
3. Prueba hacer ping al ESP32: `ping 192.168.1.105`

### Error "Token inválido":

El token JWT puede haber expirado (7 días). Cierra sesión y vuelve a iniciar sesión.

### Puerto 3000 ya está en uso:

```bash
# Cambiar puerto en docker-compose.yml
ports:
  - "3001:3000"  # Usar 3001 en lugar de 3000
```

## 📦 Producción

Para producción, considera:

1. **Cambiar secretos:**
   - Genera nuevos `JWT_SECRET` y `SESSION_SECRET`
   - Cambia credenciales de MongoDB

2. **HTTPS:**
   - Usa un proxy inverso como Nginx
   - Configura certificados SSL

3. **Variables de entorno:**
   - No uses archivos `.env` en producción
   - Usa variables de entorno del sistema o servicios como AWS Secrets Manager

4. **Backup:**
   - Configura backups automáticos de MongoDB

5. **Rate limiting:**
   - Agrega rate limiting para prevenir ataques

## 📝 Licencia

MIT

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:

1. Fork el proyecto
2. Crea una rama para tu feature
3. Commit tus cambios
4. Push a la rama
5. Abre un Pull Request

## 📧 Soporte

Para problemas o preguntas, abre un issue en el repositorio.

---

**Desarrollado con ❤️ para IoT**