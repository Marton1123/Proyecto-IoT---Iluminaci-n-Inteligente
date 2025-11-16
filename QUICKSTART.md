# ⚡ Quick Start - 5 minutos

## 🚀 Instalación Ultra Rápida

### 1️⃣ Preparar el entorno

```bash
# Clonar o crear carpeta del proyecto
mkdir iot-lighting && cd iot-lighting

# Dar permisos al script de gestión
chmod +x manage.sh
```

### 2️⃣ Configurar ESP32

Edita `backend/.env`:

```bash
ESP32_IP=192.168.1.105  # ← Cambia esto por la IP de tu ESP32
```

### 3️⃣ Levantar el sistema

```bash
# Opción 1: Usar el script de gestión (recomendado)
./manage.sh start

# Opción 2: Usar docker-compose directamente
docker-compose up -d
```

### 4️⃣ Acceder a la aplicación

- **Web App**: http://localhost:3000/login.html
- **Usuario**: `admin`
- **Password**: `admin123`

¡Listo! 🎉

---

## 📋 Comandos Básicos

```bash
# Ver estado
./manage.sh status

# Ver logs en tiempo real
./manage.sh logs

# Ver logs solo del backend
./manage.sh logs backend

# Detener todo
./manage.sh stop

# Reiniciar
./manage.sh restart

# Backup de la BD
./manage.sh backup

# Limpiar todo (cuidado!)
./manage.sh clean
```

---

## 🔧 Verificar que todo funciona

### 1. Verificar servicios corriendo

```bash
docker-compose ps
```

Deberías ver:
- ✅ `iot_mongodb` - Up
- ✅ `iot_backend` - Up
- ✅ `iot_mongo_express` - Up

### 2. Probar el backend

```bash
curl http://localhost:3000/api/ping
```

### 3. Acceder a Mongo Express

http://localhost:8081
- User: `admin`
- Pass: `admin`

---

## 🐛 Solución rápida de problemas

### El puerto 3000 ya está en uso

```bash
# Editar docker-compose.yml y cambiar el puerto
ports:
  - "3001:3000"  # Usar 3001
```

### No se conecta al ESP32

1. Verifica la IP: `ping 192.168.1.105`
2. Revisa `backend/.env`
3. Reinicia: `./manage.sh restart`

### Ver logs de errores

```bash
./manage.sh logs backend
```

---

## 📱 Probar sin ESP32

Si no tienes el ESP32 conectado, la app funcionará igual pero mostrará "ESP32 offline". Podrás:
- ✅ Login/Registro
- ✅ Ver interfaz
- ✅ Gestionar usuarios
- ❌ No habrá datos de sensores

---

## 🔐 Seguridad Básica

### Cambiar contraseña del admin

1. Login con admin/admin123
2. Ir a perfil (próximamente)
3. O usar MongoDB:

```bash
./manage.sh shell-db
# En el shell de MongoDB:
db.users.updateOne(
  { username: "admin" },
  { $set: { password: "nuevo_hash_bcrypt" } }
)
```

### Cambiar secretos JWT

Edita `backend/.env`:

```bash
JWT_SECRET=tu_secreto_super_seguro_aleatorio_largo
SESSION_SECRET=otro_secreto_diferente_tambien_largo
```

Luego reinicia:

```bash
./manage.sh restart
```

---

## 📊 Monitoreo

### Ver recursos usados

```bash
./manage.sh metrics
```

### Logs en tiempo real

```bash
./manage.sh logs
```

### Abrir shell del backend

```bash
./manage.sh shell-be
```

---

## 🎯 Próximos pasos

1. **Personaliza los umbrales** de iluminación
2. **Crea usuarios adicionales** desde la app
3. **Configura backups automáticos**
4. **Implementa HTTPS** para producción
5. **Agrega más sensores** al ESP32

---

## 📚 Más información

- Ver `README.md` para documentación completa
- API docs: http://localhost:3000/api (próximamente)
- Logs: `./manage.sh logs`

---

**¿Necesitas ayuda?** Abre un issue en el repositorio.

Happy Hacking! 🚀