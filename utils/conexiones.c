#include "conexiones.h"
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <commons/log.h> 
#include <unistd.h>
#include "serializacion.h"

extern t_log* logger;

int iniciar_servidor(char *ip, char *puerto) {
    int fd_server, err;
    struct addrinfo hints, *server_info;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(ip, puerto, &hints, &server_info);
    if (err != 0) {
        log_error(logger, "Error en getaddrinfo: %s", gai_strerror(err));
        return -1;
    }    

    fd_server = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (fd_server == -1) {
        log_error(logger, "Error al crear socket: %s", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }
    
    err = bind(fd_server, server_info->ai_addr, server_info->ai_addrlen);
    if (err == -1) {
        log_error(logger, "Error en bind: %s", strerror(errno));
        close(fd_server);
        freeaddrinfo(server_info);
        return -1;
    }
    
    err = listen(fd_server, SOMAXCONN);
    if (err == -1) {
        log_error(logger, "Error en listen: %s", strerror(errno));
        close(fd_server);
        freeaddrinfo(server_info);
        return -1;
    }
    
    freeaddrinfo(server_info);
    return fd_server;
}

int conectar_a_servidor(char *ip, char* puerto) {
    struct addrinfo hints;
    struct addrinfo *server_info;
    int fd_client, err;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(ip, puerto, &hints, &server_info);
    if (err != 0) {
        log_error(logger, "Error en getaddrinfo: %s", gai_strerror(err));
        return -1;
    }    
    
    fd_client = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (fd_client == -1) {
        log_error(logger, "Error al crear socket: %s", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    err = connect(fd_client, server_info->ai_addr, server_info->ai_addrlen);
    if (err == -1) {
        log_error(logger, "Error en connect: %s", strerror(errno));
        close(fd_client);
        freeaddrinfo(server_info);
        return -1;
    }

    freeaddrinfo(server_info);
    return fd_client;
}

int esperar_cliente(int fd_server) {
    int fd_client = accept(fd_server, NULL, NULL);
    if (fd_client == -1) {
        log_error(logger, "Error en accept: %s", strerror(errno));
        return -1;
    }
    return fd_client;
}

bool enviar_handshake(int fd, t_header handshake) {
    if (!enviar_header(handshake, fd)){
        return false;
    }
    return true;
}

bool recibir_respuesta_handshake(int fd) {
    t_header header;
    
    int bytes_recibidos = recv(fd, &header, sizeof(int), 0);
    if (bytes_recibidos != sizeof(int)) {
        return false;
    }

    if (header != HANDSHAKE_OK) {
        return false;
    }

    return true;
}
