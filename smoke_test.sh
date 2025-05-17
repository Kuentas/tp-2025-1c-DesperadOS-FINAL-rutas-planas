#!/bin/bash
set -e

# Ruta base (ajustar si se mueve el script)
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPU_DIR="$BASE_DIR/cpu"
MEM_DIR="$BASE_DIR/memoria"
KERNEL_DIR="$BASE_DIR/kernel"

# Compilar CPU, MEMORIA y KERNEL mínimo
echo "[INFO] Compilando módulos..."
make -C "$CPU_DIR" clean all
make -C "$MEM_DIR" clean all

# Compilar kernel_min.c si no existe binario
if [ ! -f "$KERNEL_DIR/bin/kernel_min" ]; then
  echo "[INFO] Compilando kernel mínimo..."
  gcc -o "$KERNEL_DIR/bin/kernel_min" \
    "$KERNEL_DIR/src/kernel_min.c" \
    "$KERNEL_DIR/../utils/conexiones.c" \
    "$KERNEL_DIR/../utils/serializacion.c" \
    -lcommons -lpthread
fi

# Lanzar MEMORIA
echo "[INFO] Iniciando MEMORIA..."
"$MEM_DIR/bin/memoria" "$MEM_DIR/config/memoria.config" &> "$MEM_DIR/memoria.log" &
MEM_PID=$!

sleep 1

# Lanzar CPU
echo "[INFO] Iniciando CPU..."
"$CPU_DIR/bin/cpu" 0 "$CPU_DIR/config/cpu.config" &> "$CPU_DIR/cpu.log" &
CPU_PID=$!

sleep 1

# Lanzar kernel mínimo que envía un proceso a CPU
echo "[INFO] Enviando proceso desde Kernel mínimo..."
"$KERNEL_DIR/bin/kernel_min"

# Esperar procesos
sleep 2

# Limpiar
echo "[INFO] Finalizando procesos..."
kill $CPU_PID $MEM_PID
wait $CPU_PID $MEM_PID 2>/dev/null || true

echo "[OK] Test finalizado. Revisar logs en cpu.log y memoria.log"
