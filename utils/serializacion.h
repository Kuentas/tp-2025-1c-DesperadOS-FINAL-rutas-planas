#ifndef SERIALIZACION_UTILS_H
#define SERIALIZACION_UTILS_H

#include <stdbool.h>

typedef enum {
    INSTRUCCION_NOOP,
    INSTRUCCION_WRITE,
    INSTRUCCION_FILE,
    INSTRUCCION_READ,
    INSTRUCCION_GOTO,
    INSTRUCCION_IO,
    INSTRUCCION_INIT_PROC,
    INSTRUCCION_DUMP_MEMORY,
    INSTRUCCION_EXIT,
    
    HANDSHAKE_MEMORIA,
    HANDSHAKE_KERNEL,
    HANDSHAKE_CPU,
    HANDSHAKE_IO,
    HANDSHAKE_OK,
    HANDSHAKE_ERROR,

    CREAR_PROCESO,
    ELIMINAR_PROCESO,
    SWAP_IN,
    SWAP_OUT,

    MEMORY_DUMP,
    ESPACIO_LIBRE,
    ERROR_MEMORIA_LLENA,
    ERROR_INSTRUCCION,
    RECIBIR_PROCESO,
    MEMORIA_SUFICIENTE,
    MEMORIA_INSUFICIENTE,
    SOLICITUD_INSTRUCCION,
    SOLICITUD_WRITE,
    SOLICITUD_READ,
    SYSCALL_IO,
    SYSCALL_INIT_PROC,
    SYSCALL_DUMP_MEMORY,
    SYSCALL_EXIT,
    
} t_header;

typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct {
	t_header header;
	t_buffer* buffer;
} t_paquete;



void* serializar_paquete(t_paquete* paquete, int bytes);
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(t_header header);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
bool enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
bool enviar_header(t_header header, int socket_cliente);
void* recibir_payload(int fd);

#endif