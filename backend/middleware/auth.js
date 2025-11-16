const jwt = require('jsonwebtoken');
const User = require('../models/User');

// Verificar token JWT
exports.authenticate = async (req, res, next) => {
  try {
    // Obtener token del header
    const authHeader = req.headers.authorization;
    
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({ 
        error: 'Acceso denegado',
        message: 'Token no proporcionado' 
      });
    }
    
    const token = authHeader.split(' ')[1];
    
    // Verificar token
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    
    // Buscar usuario
    const user = await User.findById(decoded.userId).select('-password');
    
    if (!user || !user.isActive) {
      return res.status(401).json({ 
        error: 'Acceso denegado',
        message: 'Usuario no válido' 
      });
    }
    
    // Agregar usuario al request
    req.user = user;
    next();
    
  } catch (error) {
    if (error.name === 'JsonWebTokenError') {
      return res.status(401).json({ 
        error: 'Token inválido',
        message: 'El token proporcionado no es válido' 
      });
    }
    if (error.name === 'TokenExpiredError') {
      return res.status(401).json({ 
        error: 'Token expirado',
        message: 'Tu sesión ha expirado, por favor inicia sesión nuevamente' 
      });
    }
    
    res.status(500).json({ 
      error: 'Error de autenticación',
      message: error.message 
    });
  }
};

// Verificar rol de administrador
exports.requireAdmin = (req, res, next) => {
  if (req.user.role !== 'admin') {
    return res.status(403).json({ 
      error: 'Acceso denegado',
      message: 'Se requieren privilegios de administrador' 
    });
  }
  next();
};