#include "memoria.h"
#include "utils/conexiones.h"
#include "utils/serializacion.h"
#include "utils/deserializacion.h"
#include "commons/log.h"
#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>


extern t_log* logger;

static int _conectar_con_memoria() {
    char* ip_memoria = obtener_ip_memoria();
    char* puerto_memoria = obtener_puerto_memoria();
    int fd_memoria = conectar_a_servidor(ip_memoria, puerto_memoria);

    if (fd_memoria < 0) {
        log_error(logger, "Error al intentar conectar con Memoria en %s:%s.", ip_memoria, puerto_memoria);
        return -1;
    }

    if (!enviar_handshake(fd_memoria, HANDSHAKE_KERNEL)){
        log_error(logger, "Error al enviar el Handshake a Memoria.");
        close(fd_memoria);
        return -1;
    }

    if (!recibir_respuesta_handshake(fd_memoria)) {
        log_error(logger, "No se ha podido establecer el Handshake con Memoria.");
        close(fd_memoria);
        return -1;
    }

    log_debug(logger, "Conexion establecida con Memoria en %s:%s.", ip_memoria, puerto_memoria);
    
    return fd_memoria;
}

t_header enviar_proceso_a_memoria(char* path_file, int size_file){
    int fd_memoria = _conectar_con_memoria();

    if (fd_memoria == -1) {
        exit(EXIT_FAILURE);
    }
    
    t_paquete* paquete = crear_paquete(CREAR_PROCESO);
    agregar_a_paquete(paquete, path_file, strlen(path_file)+1);
    agregar_a_paquete(paquete, &size_file, sizeof(int));
    enviar_paquete(paquete, fd_memoria);

    t_header header = obtener_header(fd_memoria);

    close(fd_memoria);
    
    return header;
}