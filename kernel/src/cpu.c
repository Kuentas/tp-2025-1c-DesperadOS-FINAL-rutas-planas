#include <pthread.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>

#include "cpu.h"
#include "config.h"
#include "utils/serializacion.h"
#include "utils/conexiones.h"

typedef struct {
    int id;
    int fd_dispatch;
    int fd_interrupt;
    bool en_uso;
} t_cpu_info;

typedef struct {
    int fd_servidor_dispatch; 
    int fd_servidor_interrupt;
    t_list* lista_cpus;
} t_cpu_manager;

extern t_log* logger;

static int _iniciar_servidor_dispatch(){
    char* puerto_escucha_dispatch = obtener_puerto_escucha_dispatch();
    int fd_servidor_dispatch = iniciar_servidor("127.0.0.1", puerto_escucha_dispatch);  
    if(fd_servidor_dispatch == -1) {
        log_error(logger, "No se ha podido inicializar el servidor Dispatch en el puerto: %s.", puerto_escucha_dispatch);
        return -1;
    }

    log_debug(logger, "Servidor Dispatch Inicializado en el puerto: %s.", puerto_escucha_dispatch);
    
    return fd_servidor_dispatch;
} 

static int _iniciar_servidor_interrupt(){
    char* puerto_escucha_interrupt = obtener_puerto_escucha_interrupt();
    int fd_servidor_interrupt = iniciar_servidor("127.0.0.1", puerto_escucha_interrupt);  
    if(fd_servidor_interrupt == -1) {
        log_error(logger, "No se ha podido inicializar el servidor Interrupt en el puerto: %s.", puerto_escucha_interrupt);
        return -1;
    }
    
    log_debug(logger, "Servidor Interrupt Inicializado en el puerto: %s.", puerto_escucha_interrupt);
    
    return fd_servidor_interrupt;
}

static void* _atender_procesos(void* arg){
    t_cpu_info* cpu = (t_cpu_info*)arg; 
    t_paquete* paquete = crear_paquete(INSTRUCCION_INIT_PROC);
    
    int pid = 10;
    int pc = 1000;
    
    agregar_a_paquete(paquete, &pid, sizeof(pid));
    agregar_a_paquete(paquete, &pc, sizeof(int)); 
    enviar_paquete(paquete, cpu->fd_interrupt);
    return NULL;
}

static void* _atender_interrupciones(void* arg){
    t_cpu_info* cpu = (t_cpu_info*)arg; 
    char* buffer = "Hola desde atender_interrupciones";
    int size = strlen(buffer) + 1;
    send(cpu->fd_interrupt, &size, sizeof(int), 0);
    send(cpu->fd_interrupt, buffer, size, 0);
    
    return NULL;
}

static void* _esperar_cpus(void* arg){
    t_cpu_manager* cpu_manager = (t_cpu_manager*) arg;
    
    while (true) {
        int fd_dispatch = esperar_cliente(cpu_manager->fd_servidor_dispatch); 
        int fd_interrupt = esperar_cliente(cpu_manager->fd_servidor_interrupt);
        
        if (fd_dispatch == -1){
            log_error(logger, "Error en el intento de conexion de Dispatch.");
            continue;
        }
        
        if (fd_interrupt == -1){
            log_error(logger, "Error en el intento de conexion de Interrupt.");
            continue;
        }

        t_cpu_info* cpu = malloc(sizeof(t_cpu_info));
        cpu->fd_dispatch = fd_dispatch;
        cpu->fd_interrupt = fd_interrupt;
        cpu->en_uso = false;
        list_add(cpu_manager->lista_cpus, cpu);
        
        log_debug(logger, "CPU conectada: dispatch=%d, interrupt=%d, cantidad de cpu's=%d", cpu->fd_dispatch, cpu->fd_interrupt, cpu_manager->lista_cpus->elements_count);
        
        pthread_t thread_procesos; 
        pthread_t thread_interrupciones;
        pthread_create(&thread_procesos, NULL, _atender_procesos, cpu);
        pthread_create(&thread_interrupciones, NULL, _atender_interrupciones, cpu);
        pthread_detach(thread_procesos);
        pthread_detach(thread_interrupciones);
    }

    return NULL;
}

void inicializar_cpu(){
    int fd_server_dispatch = _iniciar_servidor_dispatch();
    int fd_server_interrupt = _iniciar_servidor_interrupt();
    
    if (fd_server_dispatch == -1 || fd_server_interrupt == -1) {
        exit(EXIT_FAILURE);
    }

    pthread_t thread_cpus;
    t_cpu_manager* cpu_manager = malloc(sizeof(t_cpu_manager));
    cpu_manager->fd_servidor_dispatch = fd_server_dispatch;
    cpu_manager->fd_servidor_interrupt = fd_server_interrupt;
    cpu_manager->lista_cpus = list_create();
    pthread_create(&thread_cpus, NULL, _esperar_cpus, cpu_manager);
    pthread_detach(thread_cpus);
}