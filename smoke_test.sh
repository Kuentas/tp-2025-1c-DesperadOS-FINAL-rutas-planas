#!/bin/bash
set -e

echo "[INFO] Inicializando entorno de prueba..."

# Detectar ruta base
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPU_DIR="$BASE_DIR/cpu"
MEM_DIR="$BASE_DIR/memoria"
KERNEL_DIR="$BASE_DIR/kernel"
UTILS_DIR="$BASE_DIR/utils"
LOGS_DIR="$BASE_DIR/logs"

mkdir -p "$LOGS_DIR"

# Compilar CPU
echo "[INFO] Compilando CPU..."
make -C "$CPU_DIR" clean all

# Compilar Memoria
echo "[INFO] Compilando MEMORIA..."
make -C "$MEM_DIR" clean all

# Compilar kernel mínimo con utils
echo "[INFO] Compilando kernel mínimo..."
gcc -o "$KERNEL_DIR/bin/kernel_min" \
  "$KERNEL_DIR/src/kernel_min.c" \
  "$UTILS_DIR/conexiones.c" \
  "$UTILS_DIR/serializacion.c" \
  -I"$UTILS_DIR" \
  -lcommons -lpthread

# Lanzar MEMORIA
echo "[INFO] Iniciando MEMORIA..."
"$MEM_DIR/bin/memoria" "$MEM_DIR/config/memoria.config" &> "$LOGS_DIR/memoria.log" &
MEM_PID=$!
sleep 1

# Lanzar CPU
echo "[INFO] Iniciando CPU..."
"$CPU_DIR/bin/cpu" 0 "$CPU_DIR/config/cpu.config" &> "$LOGS_DIR/cpu.log" &
CPU_PID=$!
sleep 1

# Enviar proceso desde kernel mínimo
echo "[INFO] Enviando proceso desde Kernel mínimo..."
"$KERNEL_DIR/bin/kernel_min" &> "$LOGS_DIR/kernel_min.log"

# Esperar un poco
sleep 2

# Terminar procesos
echo "[INFO] Finalizando procesos..."
kill $CPU_PID $MEM_PID 2>/dev/null || true
wait $CPU_PID $MEM_PID 2>/dev/null || true

echo "[✅ OK] Test finalizado. Revisar logs en carpeta: $LOGS_DIR"

