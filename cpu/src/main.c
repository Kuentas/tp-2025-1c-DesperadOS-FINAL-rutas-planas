#include "utils/conexiones.h"
#include "commons/log.h"
#include "commons/config.h"
#include "commons/string.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include "pthread.h"
#include <stdint.h>
#include "utils/serializacion.h"
#include <string.h>

t_log* logger_general;
t_log* logger;

int fd_dispatch;
int fd_interrupt;
int fd_memoria;
pthread_t hilo_proceso;
pthread_t hilo_interrupt;
pthread_t hilo_instruccion;
bool checkInterrupt = false;
int razon = -1 ;

#define MOTIVO_INTERRUPCION "INTERRUPCION_POR_DESALOJO"

typedef struct{
    int pid;
    int pc;                
} t_proceso;

t_proceso proceso; 

void* recibir_proceso(void* arg);
void* recibir_interrupcion(void* arg);
void* recibir_instruccion(void* arg);
bool decode_execute(char* instruccion, int pid, int fd_dispatch, int fd_memoria, int* pc, t_log* logger);
bool check_interrupt(int fd_interrupt, int pid, int pc, int fd_dispatch, t_log* logger);
void ejecutar_proceso(int fd_memoria, int fd_dispatch,int fd_interrupt, int pid, t_log* logger);
int identificar_instruccion(char* instruccion);

int main(int argc, char* argv[]) {
    t_config* config = config_create("config/cpu.config");
    char* ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    char* ip_kernel = config_get_string_value(config,"IP_KERNEL");
    char* puerto_kernel_dispatch = config_get_string_value(config,"PUERTO_KERNEL_DISPATCH");
    char* puerto_kernel_interrupt = config_get_string_value(config,"PUERTO_KERNEL_INTERRUPT");
    char* entradas_tlb = config_get_string_value(config,"ENTRADAS_TLB");
    char* reemplazo_tlb = config_get_string_value(config,"REEMPLAZO_TLB");
    char* entradas_cache = config_get_string_value(config,"ENTRADAS_CACHE");
    char* reemplazo_cache = config_get_string_value(config,"REEMPLAZO_CACHE");
    char* retardo_cache = config_get_string_value(config,"RETARDO_CACHE");
    char* log_level = config_get_string_value(config,"LOG_LEVEL"); 

    logger_general = log_create("logs/cpu.log", "cpu", true, log_level_from_string(log_level));
    if (argc == 1) {
        log_error(logger_general, "No se ha introducido el ID del CPU.");
        return 0;
    }

    char* path_log = string_new();
    string_append(&path_log, "logs/cpu-");
    string_append(&path_log, argv[1]);
    string_append(&path_log, ".log");
    
    logger = log_create(path_log, "CPU", true, log_level_from_string(log_level));
    log_debug(logger, "Iniciando CPU con ID %s", argv[1]);

    fd_memoria = conectar_a_servidor(ip_memoria,puerto_memoria); // va a recibir el pc o la sig intruccion (ver que recibe)
    if(fd_memoria < 0) {
        log_error(logger, "No se ha podido inicializar conexion con Memoria.");
        log_destroy(logger);
        return EXIT_FAILURE;
    }
    log_debug(logger, "Conexion establecida con Memoria.");
    
    fd_dispatch = conectar_a_servidor(ip_kernel, puerto_kernel_dispatch);  // por este socket el kernel envia PID y un PC. Tambien el cpu envia al kernel la razon de por que lo desaloja.
    if(fd_dispatch < 0) {
        log_error(logger, "No se ha podido inicializar conexion con Dispatch.");
        log_destroy(logger);
        return EXIT_FAILURE;
    }
    log_debug(logger, "Conexion establecida con Dispatch.");

    fd_interrupt = conectar_a_servidor(ip_kernel, puerto_kernel_interrupt); // Se reciben las interrupciones, desalojo del planificador.
    if(fd_interrupt < 0) {
        log_error(logger, "No se ha podido inicializar conexion con Interrupt.");
        log_destroy(logger);
        return EXIT_FAILURE;
    }
    log_debug(logger, "Conexion establecida con Interrupt.");


    if (!enviar_handshake(fd_memoria, HANDSHAKE_CPU)){
        log_error(logger, "Error al enviar el Handshake a Memoria.");
        close(fd_memoria);
        return -1;
    }

    if (!recibir_respuesta_handshake(fd_memoria)) {
        log_error(logger, "No se ha podido establecer el Handshake con Memoria.");
        close(fd_memoria);
        return -1;
    }
   
    pthread_create(&hilo_proceso, NULL, recibir_proceso, NULL);
    pthread_create(&hilo_interrupt, NULL, recibir_interrupcion, NULL);
    pthread_create(&hilo_instruccion, NULL, recibir_instruccion, NULL);
    pthread_join(hilo_proceso, NULL);
    pthread_join(hilo_interrupt, NULL);
    pthread_join(hilo_instruccion, NULL);
    
    free(path_log);
    log_destroy(logger_general);
    log_destroy(logger);
    return 0;
}

void* recibir_proceso (void* arg){
    t_header header;
    void* payload;
    int size_payload;
    int offset = 0;
    
    recv(fd_dispatch, &header, sizeof(int), 0);
    
    switch (header) {
        case RECIBIR_PROCESO: {
            int pid;
            int size_pid;
            int pc;
            int size_pc;
            

            recv(fd_dispatch, &size_payload, sizeof(int), 0);
            payload = malloc(size_payload);
            recv(fd_dispatch, payload, size_payload, 0);

            
            memcpy(&size_pid, payload+offset, sizeof(int));
            offset = offset + sizeof(int);
            memcpy(&pid, payload+offset, size_pid);
            offset = offset + size_pid;

            memcpy(&size_pc, payload+offset, sizeof(int));
            offset = offset + sizeof(int);
            memcpy(&pc, payload+offset, size_pc);
            offset = offset + size_pc;

            log_debug(logger, "llego el proceso con pid: %i y pc %i", pid, pc);

            proceso.pid=pid;
            proceso.pc=pc;

            //fetch
            t_paquete* pedido_instruccion = crear_paquete(SOLICITUD_INSTRUCCION);
            agregar_a_paquete(pedido_instruccion,pid,sizeof(pid));
            agregar_a_paquete(pedido_instruccion,pc,sizeof(pc));
            enviar_paquete(pedido_instruccion,fd_memoria);



        }
    }
    return NULL;
}

void* recibir_interrupcion(void* arg){ // Debe recibir el numero de instruccion en este caso el Desalojo y ademas activar algun mecanismo para saber que hubo una interrucpion
    return NULL;
}

void* recibir_instruccion(void* arg){ 
    t_header header;
    int pc;
    int pid;
    int size;
    int offset = 0;
    char* payload;

    //decode and execute
    recv(fd_memoria, &header, sizeof(int), 0);
    switch (header)
    {
    case INSTRUCCION_NOOP :
        break;

   /* case INSTRUCCION_WRITE : {
        int size;
        int offset = 0;
        int direccion_fisica;
        char* payload;
        int direccion;
        char* datos;

        payload = recibir_payload(fd_memoria);

        memcpy(&size, payload + offset, sizeof(int));
        offset = offset + sizeof(int);
        
        memcpy(direccion, payload + offset, size);
        offset = offset + size;

        datos = malloc(size);
        memcpy(datos, payload + offset, size);
        offset = offset + size;
        free(payload);

        // falta pasar de dir logica a fisica

        // serializar paquete y mandarlo

        t_paquete* pedido_de_escritura= crear_paquete(SOLICITUD_WRITE);
        agregar_a_paquete(pedido_de_escritura,&pid,sizeof(direccion_fisica));
        agregar_a_paquete(pedido_de_escritura,pc,sizeof(datos));
        enviar_paquete(pedido_de_escritura,fd_memoria);

        proceso.pc= proceso.pc+1;

        t_paquete* pedido_instruccion = crear_paquete(SOLICITUD_INSTRUCCION);
        agregar_a_paquete(pedido_instruccion,&pid,sizeof(pid));
        agregar_a_paquete(pedido_instruccion,pc,sizeof(pc));
        enviar_paquete(pedido_instruccion,fd_memoria);

    } 

    case INSTRUCCION_READ : {
        int size;
        int offset = 0;
        char* payload;
        int direccion;
        int tamanio;

        payload = recibir_payload(fd_memoria);

        memcpy(&size, payload + offset, sizeof(int));
        offset = offset + sizeof(int);
        
        memcpy(&direccion, payload + offset, size);
        offset = offset + size;

        memcpy(&size, payload + offset, sizeof(int));
        offset = offset + sizeof(int);
        
        memcpy(&tamanio, payload + offset, size);
        offset = offset + size;
        free(payload);

        t_paquete* pedido_de_lectura= crear_paquete(SOLICITUD_READ);
        agregar_a_paquete(pedido_de_lectura,direcion_fisica,sizeof(direccion_fisica));
        agregar_a_paquete(pedido_de_lectura,tamanio,sizeof(tamanio));
        enviar_paquete(pedido_de_lectura,fd_memoria);

        proceso.pc= proceso.pc+1;

        t_paquete* pedido_instruccion = crear_paquete(SOLICITUD_INSTRUCCION);
        agregar_a_paquete(pedido_instruccion,pid,sizeof(pid));
        agregar_a_paquete(pedido_instruccion,pc,sizeof(pc));
        enviar_paquete(pedido_instruccion,fd_memoria);

    }
*/
    case INSTRUCCION_GOTO:{
        int direccion_memoria;

        payload = recibir_payload(fd_memoria);

        memcpy(&size, payload + offset, sizeof(int));
        offset = offset + sizeof(int);

        memcpy(&direccion_memoria, payload + offset, size);
        offset = offset + size;
        free(payload);

        proceso.pc=direccion_memoria;

        t_paquete* pedido_instruccion = crear_paquete(SOLICITUD_INSTRUCCION);
        agregar_a_paquete(pedido_instruccion,pid,sizeof(pid));
        agregar_a_paquete(pedido_instruccion,pc,sizeof(pc));
        enviar_paquete(pedido_instruccion,fd_memoria);
    

    } 
    case INSTRUCCION_IO: {
            char* dispositivo;
            int tiempo_bloqueo;

            payload = recibir_payload(fd_memoria);

            memcpy(&size, payload + offset, sizeof(int));
            offset = offset + sizeof(int);

            memcpy(&dispositivo, payload + offset, size);
            offset = offset + size;

            memcpy(&size, payload + offset, sizeof(int));
            offset = offset + sizeof(int);

            memcpy(&tiempo_bloqueo, payload + offset, size);
            offset = offset + size;
            free(payload);
        
            log_info(logger, "PID %d ejecutó IO por %d unidades de tiempo", proceso.pid, tiempo_bloqueo);
        
            t_paquete* paquete_io = crear_paquete(SYSCALL_IO);
            agregar_a_paquete(paquete_io, &proceso.pid, sizeof(int));
            agregar_a_paquete(paquete_io, &dispositivo, sizeof(int));
            agregar_a_paquete(paquete_io, &tiempo_bloqueo, sizeof(int));
            enviar_paquete(paquete_io, fd_dispatch);
        
            pthread_exit(NULL); // Termina el hilo, ya que el proceso fue desalojado
        }
        

        case INSTRUCCION_INIT_PROC: {
            FILE* archivo_instrucciones;
            int tamanio;
        
            payload = recibir_payload(fd_memoria);

            memcpy(&size, payload + offset, sizeof(int));
            offset = offset + sizeof(int);

            memcpy(&archivo_instrucciones, payload + offset, size);
            offset = offset + size;

            memcpy(&size, payload + offset, sizeof(int));
            offset = offset + sizeof(int);

            memcpy(&tamanio, payload + offset, size);
            offset = offset + size;
            free(payload);
        
            log_info(logger, "PID %d solicitó INIT_PROC con PID hijo %d", proceso.pid, archivo_instrucciones);
        
            t_paquete* paquete_init = crear_paquete(SYSCALL_INIT_PROC);
            agregar_a_paquete(paquete_init, &proceso.pid, sizeof(int));
            agregar_a_paquete(paquete_init, &archivo_instrucciones, sizeof(int));
            agregar_a_paquete(paquete_init, &tamanio, sizeof(int));
            enviar_paquete(paquete_init, fd_dispatch);
        
            pthread_exit(NULL); // Proceso se bloquea esperando al hijo
        }
        
        case INSTRUCCION_DUMP_MEMORY: {
            log_info(logger, "PID %d solicitó DUMP_MEMORY", proceso.pid);
        
            t_paquete* paquete_dump = crear_paquete(SYSCALL_DUMP_MEMORY);
            agregar_a_paquete(paquete_dump, &proceso.pid, sizeof(int));
            enviar_paquete(paquete_dump, fd_dispatch);
        
            proceso.pc += 1;
        
            t_paquete* pedido_instruccion = crear_paquete(SOLICITUD_INSTRUCCION);
            agregar_a_paquete(pedido_instruccion, &proceso.pid, sizeof(int));
            agregar_a_paquete(pedido_instruccion, &proceso.pc, sizeof(int));
            enviar_paquete(pedido_instruccion, fd_memoria);
            break;
        }

        case INSTRUCCION_EXIT: {
            log_info(logger, "PID %d finalizó ejecución con EXIT", proceso.pid);
        
            t_paquete* paquete_exit = crear_paquete(SYSCALL_EXIT);
            agregar_a_paquete(paquete_exit, &proceso.pid, sizeof(int));
            enviar_paquete(paquete_exit, fd_dispatch);
        
            pthread_exit(NULL); // Finaliza el hilo del proceso
        }
        
    
    default:
        break;
    }
    return NULL;
}


bool check_interrupt(int fd_interrupt, int pid, int pc, int fd_dispatch, t_log* logger) {
    char buffer[64];
    int bytes = recv(fd_interrupt, buffer, sizeof(buffer), MSG_DONTWAIT);

    if (bytes > 0 && strncmp(buffer, "INTERRUPT", 9) == 0) {
        log_info(logger, "Interrupción recibida para PID %d", pid);
        char respuesta[128];
        sprintf(respuesta, "INTERRUPCION PID %d PC %d MOTIVO %s", pid, pc, MOTIVO_INTERRUPCION);
        send(fd_dispatch, respuesta, strlen(respuesta) + 1, 0);
        return true;
    }
    return false;
}









