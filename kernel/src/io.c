#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <commons/collections/list.h>
#include <commons/log.h>
#include "utils/conexiones.h"
#include "config.h"

extern t_log* logger;

typedef struct {
    int fd_io;
    char* nombre;
    bool en_uso;
} t_io_info;

typedef struct {
    int fd_servidor_io;
    t_list* lista_ios;
} t_io_manager;

static int _iniciar_servidor_io(){
    char* puerto_escucha_io = obtener_puerto_escucha_io();
    int fd_servidor_io = iniciar_servidor("127.0.0.1", puerto_escucha_io);  
    if(fd_servidor_io == -1) {
        log_error(logger, "No se ha podido inicializar el servidor IO en el puerto: %s.", puerto_escucha_io);
        return -1;
    }
    
    log_debug(logger, "Servidor IO Inicializado en el puerto: %s.", puerto_escucha_io);
    
    return fd_servidor_io;
}

static void* _atender_io(void* arg){
    t_io_info* io = (t_io_info*)arg;
    
    int size;
    recv(io->fd_io, &size, sizeof(int), 0);
    char* nombre = malloc(size);
    recv(io->fd_io, nombre, size, 0);
    
    io->nombre = nombre;

    log_debug(logger, "Diapositivo IO conectado: %s.", io->nombre);

    int tiempo = 10;
    send(io->fd_io, &tiempo, sizeof(int), 0);

    int bytes_recibidos = recv(io->fd_io, &tiempo, sizeof(int), 0);
    while (bytes_recibidos > 0) { // Al momento de recibir un mensaje de una IO se deberá verificar que el mismo sea una confirmación de fin de IO, en caso afirmativo, se deberá validar si hay más procesos esperando realizar dicha IO.


    } 
    
    // en caso de que el mensaje corresponda a una desconexión de la IO, el proceso que estaba ejecutando en dicha IO, se deberá pasar al estado EXIT.
    
    return NULL;
}

static void* _esperar_ios(void* arg){
    t_io_manager* io_manager = (t_io_manager*)arg;
    
    while(true) {
        int fd_io = esperar_cliente(io_manager->fd_servidor_io);
        
        t_io_info* io = malloc(sizeof(t_io_info));
        io->fd_io = fd_io;
        io->en_uso = false;
        list_add(io_manager->lista_ios, io);

        pthread_t thread_ios;
        pthread_create(&thread_ios, NULL, _atender_io, io);
        pthread_detach(thread_ios);
    }

    return NULL;
}

void inicializar_io(){
    int fd_servidor_io = _iniciar_servidor_io();
    if (fd_servidor_io == -1) {
        exit(EXIT_FAILURE);
    }


    pthread_t thread_ios;
    t_io_manager* io_manager = malloc(sizeof(t_io_manager));
    io_manager->fd_servidor_io = fd_servidor_io;
    io_manager->lista_ios = list_create();  
    pthread_create(&thread_ios, NULL, _esperar_ios, io_manager);
    
    pthread_detach(thread_ios);
}