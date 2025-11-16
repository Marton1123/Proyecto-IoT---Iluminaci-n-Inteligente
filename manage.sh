#!/bin/bash

# Script de gestión del sistema IoT Lighting

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funciones de utilidad
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Banner
show_banner() {
    echo -e "${BLUE}"
    echo "╔═══════════════════════════════════════════╗"
    echo "║     🏠 IoT Lighting System Manager      ║"
    echo "╚═══════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Verificar Docker
check_docker() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker no está instalado"
        echo "Por favor instala Docker: https://docs.docker.com/get-docker/"
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        print_error "Docker Compose no está instalado"
        echo "Por favor instala Docker Compose"
        exit 1
    fi
    
    print_success "Docker y Docker Compose están instalados"
}

# Iniciar servicios
start_services() {
    print_info "Iniciando servicios..."
    docker-compose up -d
    
    echo ""
    print_success "Servicios iniciados correctamente"
    echo ""
    echo "📡 Servicios disponibles:"
    echo "   • Frontend:      http://localhost:3000/login.html"
    echo "   • Backend API:   http://localhost:3000/api"
    echo "   • Mongo Express: http://localhost:8081"
    echo ""
    echo "👤 Usuario por defecto:"
    echo "   • Usuario: admin"
    echo "   • Password: admin123"
    echo ""
}

# Detener servicios
stop_services() {
    print_info "Deteniendo servicios..."
    docker-compose down
    print_success "Servicios detenidos"
}

# Reiniciar servicios
restart_services() {
    print_info "Reiniciando servicios..."
    docker-compose restart
    print_success "Servicios reiniciados"
}

# Ver logs
view_logs() {
    service=$1
    if [ -z "$service" ]; then
        docker-compose logs -f
    else
        docker-compose logs -f $service
    fi
}

# Estado de servicios
check_status() {
    print_info "Estado de los servicios:"
    echo ""
    docker-compose ps
}

# Limpiar todo
clean_all() {
    print_warning "Esto eliminará todos los contenedores, volúmenes y datos"
    read -p "¿Estás seguro? (y/N): " confirm
    
    if [[ $confirm == [yY] ]]; then
        print_info "Limpiando..."
        docker-compose down -v
        print_success "Limpieza completada"
    else
        print_info "Operación cancelada"
    fi
}

# Backup de MongoDB
backup_db() {
    timestamp=$(date +%Y%m%d_%H%M%S)
    backup_dir="./backups"
    mkdir -p $backup_dir
    
    print_info "Creando backup de la base de datos..."
    docker exec iot_mongodb mongodump --archive=/tmp/backup_$timestamp.archive --gzip
    docker cp iot_mongodb:/tmp/backup_$timestamp.archive $backup_dir/
    docker exec iot_mongodb rm /tmp/backup_$timestamp.archive
    
    print_success "Backup creado: $backup_dir/backup_$timestamp.archive"
}

# Restaurar MongoDB
restore_db() {
    if [ -z "$1" ]; then
        print_error "Debes especificar el archivo de backup"
        echo "Uso: $0 restore <archivo_backup>"
        exit 1
    fi
    
    if [ ! -f "$1" ]; then
        print_error "El archivo $1 no existe"
        exit 1
    fi
    
    print_warning "Esto sobrescribirá la base de datos actual"
    read -p "¿Continuar? (y/N): " confirm
    
    if [[ $confirm == [yY] ]]; then
        print_info "Restaurando backup..."
        docker cp $1 iot_mongodb:/tmp/restore.archive
        docker exec iot_mongodb mongorestore --archive=/tmp/restore.archive --gzip --drop
        docker exec iot_mongodb rm /tmp/restore.archive
        print_success "Backup restaurado correctamente"
    else
        print_info "Operación cancelada"
    fi
}

# Ver métricas
show_metrics() {
    print_info "Métricas de los contenedores:"
    echo ""
    docker stats --no-stream iot_mongodb iot_backend iot_mongo_express
}

# Shell en el backend
shell_backend() {
    print_info "Abriendo shell en el backend..."
    docker exec -it iot_backend sh
}

# Shell en MongoDB
shell_mongodb() {
    print_info "Abriendo shell de MongoDB..."
    docker exec -it iot_mongodb mongosh -u admin -p admin123 --authenticationDatabase admin iot_lighting
}

# Reconstruir backend
rebuild_backend() {
    print_info "Reconstruyendo imagen del backend..."
    docker-compose build backend
    docker-compose up -d backend
    print_success "Backend reconstruido"
}

# Mostrar ayuda
show_help() {
    echo "Uso: $0 [comando]"
    echo ""
    echo "Comandos disponibles:"
    echo "  start         - Iniciar todos los servicios"
    echo "  stop          - Detener todos los servicios"
    echo "  restart       - Reiniciar todos los servicios"
    echo "  status        - Ver estado de los servicios"
    echo "  logs [srv]    - Ver logs (opcional: backend/mongodb/mongo-express)"
    echo "  clean         - Eliminar todo (contenedores, volúmenes, datos)"
    echo "  backup        - Crear backup de MongoDB"
    echo "  restore <f>   - Restaurar backup de MongoDB"
    echo "  metrics       - Ver métricas de recursos"
    echo "  shell-be      - Abrir shell en el backend"
    echo "  shell-db      - Abrir shell de MongoDB"
    echo "  rebuild       - Reconstruir imagen del backend"
    echo "  help          - Mostrar esta ayuda"
    echo ""
}

# Main
show_banner

case "$1" in
    start)
        check_docker
        start_services
        ;;
    stop)
        stop_services
        ;;
    restart)
        restart_services
        ;;
    status)
        check_status
        ;;
    logs)
        view_logs $2
        ;;
    clean)
        clean_all
        ;;
    backup)
        backup_db
        ;;
    restore)
        restore_db $2
        ;;
    metrics)
        show_metrics
        ;;
    shell-be)
        shell_backend
        ;;
    shell-db)
        shell_mongodb
        ;;
    rebuild)
        rebuild_backend
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        if [ -z "$1" ]; then
            show_help
        else
            print_error "Comando desconocido: $1"
            echo ""
            show_help
        fi
        exit 1
        ;;
esac