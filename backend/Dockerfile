FROM node:18-alpine

WORKDIR /usr/src/app

# Copiar package.json del backend
COPY backend/package*.json ./

# Instalar dependencias
RUN npm install --production

# Copiar código del backend
COPY backend/server.js ./
COPY backend/models ./models
COPY backend/middleware ./middleware

# Copiar frontend directamente al contenedor
COPY frontend/public ./public

# Crear directorio para logs (opcional)
RUN mkdir -p logs

EXPOSE 3000

ENV NODE_ENV=production

CMD ["node", "server.js"]