#ifndef CONEXIONES_UTILS_H
#define CONEXIONES_UTILS_H

#include <stdbool.h>
#include "serializacion.h"

/**
 * @brief Inicia un servidor TCP en la dirección IP y puerto especificados.
 * 
 * Configura un socket, lo enlaza a la IP:puerto, y lo deja en modo escucha.
 * Registra errores mediante un logger declarado como EXTERN.
 * 
 * @param ip Dirección IP (ej: "0.0.0.0" para todas las interfaces). Si es NULL, usa ANY.
 * @param puerto Puerto en formato string (ej: "8080").
 * 
 * @return File descriptor del socket servidor en éxito, -1 en error.
 * 
 * @note El caller debe cerrar el socket retornado
 */
int iniciar_servidor(char *ip, char *puerto);

/**
 * @brief Establece una conexión TCP con un servidor remoto.
 * 
 * @param ip Dirección IP del servidor (ej: "127.0.0.1").
 * @param puerto Puerto en formato string (ej: "8080").
 * 
 * @return File descriptor del socket cliente en éxito, -1 en error.
 */
int conectar_a_servidor(char *ip, char *puerto);

/**
 * @brief Acepta una conexión entrante en un socket servidor.
 * 
 * @param fd_server File descriptor del socket servidor (previamente configurado con listen()).
 * 
 * @return File descriptor del socket cliente aceptado, -1 en error.
 * 
 * @warning Esta función es BLOQUEANTE. No retornará hasta que haya una conexión o falle.
 */
int esperar_cliente(int fd_server);


bool enviar_handshake(int fd, t_header handshake);\
bool recibir_respuesta_handshake(int fd);

#endif // CONEXIONES_H