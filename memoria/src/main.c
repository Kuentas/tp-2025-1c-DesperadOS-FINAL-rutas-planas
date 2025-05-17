#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "utils/conexiones.h"
#include <stdint.h>
#include "utils/serializacion.h"
#include <string.h>
#include <stdio.h>

// Estructuras de datos
typedef struct {
    int pid;
    int tamanio;
    int accesos_tabla_paginas;
    int instrucciones_sol;
    int swap_ops;
    int mem_principal_ops;
    int lecturas_mem;
    int escrituras_mem;
    t_list* instrucciones;
} t_proceso;

// Variables globales
t_log* logger;
int fd_servidor_memoria;
int retardo_memoria;
t_list* procesos;
int espacio_total_mock = 1024;
int espacio_libre_mock = 1024;

// Prototipos de funciones
void* esperar_clientes(void* arg);
void* handshake(void* arg);
void* atender_kernel(void* arg);
void* atender_cpu(void* arg);
t_proceso* crear_proceso(int pid, int tamanio, t_list* instrucciones);
void eliminar_proceso(int pid);
bool hay_espacio_suficiente(int tamanio_solicitado);
char* obtener_instruccion(int pid, int pc);
t_list* leer_instrucciones_desde_archivo(const char* path_archivo);

int main(int argc, char* argv[]) {    
    t_config* config = config_create("config/memoria.config");
    if(config == NULL) {
        printf("Error al leer el archivo de configuración\n");
        return EXIT_FAILURE;
    }

    char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    char* log_level = config_get_string_value(config,"LOG_LEVEL");
    retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");

    logger = log_create("logs/memoria.log", "memoria", true, log_level_from_string(log_level));
    procesos = list_create();

    fd_servidor_memoria = iniciar_servidor("127.0.0.1", puerto_escucha);
    if(fd_servidor_memoria == -1) {
        log_error(logger, "No se ha podido inicializar el servidor Memoria.");
        config_destroy(config);
        return EXIT_FAILURE;
    }

    log_info(logger, "Servidor Memoria Inicializado. Espacio total: %d - Libre: %d", 
             espacio_total_mock, espacio_libre_mock);
    
    pthread_t thread;
    pthread_create(&thread, NULL, esperar_clientes, NULL);
    pthread_join(thread, NULL);

    close(fd_servidor_memoria);
    config_destroy(config);
    log_destroy(logger);
    list_destroy_and_destroy_elements(procesos, (void*)eliminar_proceso);
    
    return 0;
}

// Función para leer instrucciones desde archivo
t_list* leer_instrucciones_desde_archivo(const char* path_archivo) {
    t_list* instrucciones = list_create();
    FILE* archivo = fopen(path_archivo, "r");
    
    if (archivo == NULL) {
        log_error(logger, "No se pudo abrir el archivo de instrucciones: %s", path_archivo);
        return instrucciones;
    }

    char* linea = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&linea, &len, archivo)) != -1) {
        if (linea[read - 1] == '\n') {
            linea[read - 1] = '\0';
        }
        list_add(instrucciones, strdup(linea));
    }

    free(linea);
    fclose(archivo);
    log_info(logger, "Leídas %d instrucciones desde %s", list_size(instrucciones), path_archivo);
    return instrucciones;
}

// Función para verificar espacio disponible
bool hay_espacio_suficiente(int tamanio_solicitado) {
    return tamanio_solicitado <= espacio_libre_mock;
}

// Función para crear proceso con instrucciones
t_proceso* crear_proceso(int pid, int tamanio, t_list* instrucciones) {
    if (!hay_espacio_suficiente(tamanio)) {
        log_error(logger, "Intento de crear proceso PID %d sin espacio suficiente", pid);
        return NULL;
    }
    
    t_proceso* nuevo = malloc(sizeof(t_proceso));
    nuevo->pid = pid;
    nuevo->tamanio = tamanio;
    nuevo->accesos_tabla_paginas = 0;
    nuevo->instrucciones_sol = 0;
    nuevo->swap_ops = 0;
    nuevo->mem_principal_ops = 0;
    nuevo->lecturas_mem = 0;
    nuevo->escrituras_mem = 0;
    nuevo->instrucciones = instrucciones;
    
    list_add(procesos, nuevo);
    espacio_libre_mock -= tamanio;
    
    log_info(logger, "Proceso PID %d creado - Tamaño: %d - Instrucciones: %d - Espacio libre: %d", 
             pid, tamanio, list_size(instrucciones), espacio_libre_mock);
    return nuevo;
}

// Función para eliminar proceso
void eliminar_proceso(int pid) {
    bool buscar_por_pid(void* elemento) {
        return ((t_proceso*)elemento)->pid == pid;
    }
    
    t_proceso* proc = list_remove_by_condition(procesos, buscar_por_pid);
    if (proc != NULL) {
        espacio_libre_mock += proc->tamanio;
        log_info(logger, "Proceso PID %d eliminado. Espacio liberado: %d. Nuevo espacio libre: %d",
                pid, proc->tamanio, espacio_libre_mock);
        
        list_destroy_and_destroy_elements(proc->instrucciones, free);
        free(proc);
    }
}

// Función para obtener instrucción específica
char* obtener_instruccion(int pid, int pc) {
    for (int i = 0; i < list_size(procesos); i++) {
        t_proceso* proc = list_get(procesos, i);
        if (proc->pid == pid) {
            if (pc >= 0 && pc < list_size(proc->instrucciones)) {
                proc->instrucciones_sol++;
                return list_get(proc->instrucciones, pc);
            }
            break;
        }
    }
    return NULL;
}

// Hilo para esperar conexiones de clientes
void* esperar_clientes(void* arg){
    while (true) {
        int fd_cliente = esperar_cliente(fd_servidor_memoria);        
        int* fd_cliente_ptr = malloc(sizeof(int));
        *fd_cliente_ptr = fd_cliente;
        
        pthread_t thread;
        pthread_create(&thread, NULL, handshake, fd_cliente_ptr);
        pthread_detach(thread);
    }
}

// Handshake con clientes
void* handshake(void* arg){
    int* fd_cliente_ptr = (int*)arg;
    
    t_header header;
    recv(*fd_cliente_ptr, &header, sizeof(int), 0);

    pthread_t thread_atencion;
   
    switch (header) {
        case HANDSHAKE_KERNEL: {
            enviar_header(HANDSHAKE_OK, *fd_cliente_ptr);
            log_info(logger, "## Kernel Conectado - FD: %i", *fd_cliente_ptr);
            pthread_create(&thread_atencion, NULL, atender_kernel, fd_cliente_ptr);
            break;
        }

        case HANDSHAKE_CPU: {
            enviar_header(HANDSHAKE_OK, *fd_cliente_ptr);
            log_info(logger, "## CPU Conectado - FD: %i", *fd_cliente_ptr);
            pthread_create(&thread_atencion, NULL, atender_cpu, fd_cliente_ptr);
            break;
        }

        default: {
            enviar_header(HANDSHAKE_ERROR, *fd_cliente_ptr);
            log_error(logger, "Handshake no reconocido");
            close(*fd_cliente_ptr);
            free(fd_cliente_ptr);
            break;
        }
    }
    return NULL;
}

// Atención al kernel
void* atender_kernel(void* arg) {
    int fd_kernel = ((int)arg);
    free(arg); 
    
    t_header header;
    int byte_recibidos = recv(fd_kernel, &header, sizeof(int), 0);

    while (byte_recibidos > 0) {
        switch(header) {
            case CREAR_PROCESO: {
                t_paquete* paquete = crear_paquete(0);
                recv(fd_kernel, &(paquete->buffer->size), sizeof(int), 0);
                paquete->buffer->stream = malloc(paquete->buffer->size);
                recv(fd_kernel, paquete->buffer->stream, paquete->buffer->size, 0);
                
                int offset = 0;
                int pid, tamanio;
                memcpy(&pid, paquete->buffer->stream + offset, sizeof(int));
                offset += sizeof(int);
                memcpy(&tamanio, paquete->buffer->stream + offset, sizeof(int));
                offset += sizeof(int);
                
                if (!hay_espacio_suficiente(tamanio)) {
                    log_error(logger, "No hay espacio para PID: %d - Tamaño: %d - Libre: %d",pid, tamanio, espacio_libre_mock);
                    enviar_header(ERROR_MEMORIA_LLENA, fd_kernel);
                    eliminar_paquete(paquete);
                    break;
                }
                
                int path_length;
                memcpy(&path_length, paquete->buffer->stream + offset, sizeof(int));
                offset += sizeof(int);
                
                char* path_instrucciones = malloc(path_length + 1);
                memcpy(path_instrucciones, paquete->buffer->stream + offset, path_length);
                path_instrucciones[path_length] = '\0';
                offset += path_length;
                
                t_list* instrucciones = leer_instrucciones_desde_archivo(path_instrucciones);
                crear_proceso(pid, tamanio, instrucciones);
                
                free(path_instrucciones);
                eliminar_paquete(paquete);
                enviar_header(HANDSHAKE_OK, fd_kernel);
                break;
            }

            case ELIMINAR_PROCESO: {
                int pid;
                recv(fd_kernel, &pid, sizeof(int), 0);
                eliminar_proceso(pid);
                enviar_header(HANDSHAKE_OK, fd_kernel);
                break;
            }

            case SWAP_IN: {
                t_paquete* paquete = crear_paquete(0);
                recv(fd_kernel, &(paquete->buffer->size), sizeof(int), 0);
                paquete->buffer->stream = malloc(paquete->buffer->size);
                recv(fd_kernel, paquete->buffer->stream, paquete->buffer->size, 0);
                
                int pid, pagina;
                memcpy(&pid, paquete->buffer->stream, sizeof(int));
                memcpy(&pagina, paquete->buffer->stream + sizeof(int), sizeof(int));
                
                log_info(logger, "## PID: %d - SWAP IN - Página: %d", pid, pagina);
                enviar_header(HANDSHAKE_OK, fd_kernel);
                eliminar_paquete(paquete);
                break;
            }
     
            case SWAP_OUT: {
                t_paquete* paquete = crear_paquete(0);
                recv(fd_kernel, &(paquete->buffer->size), sizeof(int), 0);
                paquete->buffer->stream = malloc(paquete->buffer->size);
                recv(fd_kernel, paquete->buffer->stream, paquete->buffer->size, 0);
                
                int pid, pagina;
                memcpy(&pid, paquete->buffer->stream, sizeof(int));
                memcpy(&pagina, paquete->buffer->stream + sizeof(int), sizeof(int));
                
                log_info(logger, "## PID: %d - SWAP OUT - Página: %d", pid, pagina);
                enviar_header(HANDSHAKE_OK, fd_kernel);
                break;
            }

            case MEMORY_DUMP: {
                int pid;
                recv(fd_kernel, &pid, sizeof(int), 0);
                log_info(logger, "Memory Dump solicitado para PID: %d", pid);
                enviar_header(HANDSHAKE_OK, fd_kernel);
                break;
            }

            case ESPACIO_LIBRE: {
                enviar_header(HANDSHAKE_OK, fd_kernel);
                send(fd_kernel, &espacio_libre_mock, sizeof(int), 0);
                break;
            }

            default: {
                log_error(logger, "Operación desconocida desde Kernel: %d", header);
                close(fd_kernel);
                return NULL;
            }
        }
        byte_recibidos = recv(fd_kernel, &header, sizeof(int), 0);
    }
    
    return NULL;
}

// Atención a la CPU
void* atender_cpu(void* arg) {
    int fd_cpu = ((int)arg);
    free(arg); 
    
    t_header header;
    int byte_recibidos = recv(fd_cpu, &header, sizeof(int), 0);

    while (byte_recibidos > 0) {
        switch(header) {
            case SOLICITUD_INSTRUCCION: {
                int pid, pc;
                recv(fd_cpu, &pid, sizeof(int), 0);
                recv(fd_cpu, &pc, sizeof(int), 0);
                
                char* instruccion = obtener_instruccion(pid, pc);
                if (instruccion != NULL) {
                    t_paquete* respuesta = crear_paquete(HANDSHAKE_OK);
                    agregar_a_paquete(respuesta, instruccion, strlen(instruccion) + 1);
                    enviar_paquete(respuesta, fd_cpu);
                    eliminar_paquete(respuesta);
                    log_info(logger, "Instrucción enviada a CPU - PID: %d - PC: %d", pid, pc);
                } else {
                    enviar_header(ERROR_INSTRUCCION, fd_cpu);
                    log_warning(logger, "Solicitud inválida - PID: %d - PC: %d", pid, pc);
                }
                break;
            }

            case SOLICITUD_READ: {
                t_paquete* paquete = crear_paquete(0);
                recv(fd_cpu, &(paquete->buffer->size), sizeof(int), 0);
                paquete->buffer->stream = malloc(paquete->buffer->size);
                recv(fd_cpu, paquete->buffer->stream, paquete->buffer->size, 0);
                
                int pid, dir_fisica, tamanio;
                int offset = 0;
                memcpy(&pid, paquete->buffer->stream + offset, sizeof(int));
                offset += sizeof(int);
                memcpy(&dir_fisica, paquete->buffer->stream + offset, sizeof(int));
                offset += sizeof(int);
                memcpy(&tamanio, paquete->buffer->stream + offset, sizeof(int));
                
                // Mock: preparar datos de respuesta
                void* datos = malloc(tamanio);
                memset(datos, 0xAA, tamanio);
                
                t_paquete* respuesta = crear_paquete(HANDSHAKE_OK);
                agregar_a_paquete(respuesta, &tamanio, sizeof(int));
                agregar_a_paquete(respuesta, datos, tamanio);
                
                log_info(logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d", 
                        pid, dir_fisica, tamanio);
                
                enviar_paquete(respuesta, fd_cpu);
                free(datos);
                eliminar_paquete(paquete);
                break;
            }

            case SOLICITUD_WRITE: {
                int pid, dir_fisica, tamanio;
                recv(fd_cpu, &pid, sizeof(int), 0);
                recv(fd_cpu, &dir_fisica, sizeof(int), 0);
                recv(fd_cpu, &tamanio, sizeof(int), 0);
                
                // Recibir datos
                void* datos = malloc(tamanio);
                recv(fd_cpu, datos, tamanio, 0);
                
                log_info(logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d", 
                        pid, dir_fisica, tamanio);
                free(datos);
                
                enviar_header(HANDSHAKE_OK, fd_cpu);
                break;
            }

            default: {
                log_error(logger, "Operación desconocida desde CPU: %d", header);
                close(fd_cpu);
                return NULL;
            }
        }
        byte_recibidos = recv(fd_cpu, &header, sizeof(int), 0);
    }
    
    return NULL;
}