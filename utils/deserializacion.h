#ifndef DESERIALIZACION_UTILS
#define DESERIALIZACION_UTILS

#include "serializacion.h" 
#include <sys/socket.h>

/**
 * @brief Obtiene un encabezado (header) recibido a través de un socket.
 * @param fd Descriptor de archivo del socket (debe ser un socket válido y conectado).
 * @return El valor del encabezado leído o -1 en caso de error.
 * @warning
 *   - Si falla, retorna -1.
 * @example
 *   t_header header = obtener_header(socket_fd);
 */
t_header obtener_header(int fd);

/**
 * @brief Obtiene un payload (datos binarios) recibido a través de un socket.
 * @param fd Descriptor del socket (file descriptor) para recibir datos.
 * @return Puntero al payload asignado en memoria dinámica (malloc) o NULL en caso de error. 
 * @warning 
 *  Si falla, retorna NULL.
 *  ¡Debe liberarse con free() después de usarse!
 * @example
 *   void* payload = obtener_payload(socket_fd);
 *   free(payload); // ¡Liberar memoria!
 */
void* obtener_payload(int fd);

#endif