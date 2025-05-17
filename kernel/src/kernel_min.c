#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "conexiones.h"
#include "serializacion.h"

#define IP_CPU "127.0.0.1"
#define PUERTO_DISPATCH "8001"

int main() {
    int conexion_cpu = conectar_a_servidor(IP_CPU, PUERTO_DISPATCH);
    if (conexion_cpu < 0) {
        fprintf(stderr, "[ERROR] No se pudo conectar a CPU.\n");
        return EXIT_FAILURE;
    }

    printf("[INFO] Conectado a CPU. Enviando proceso...\n");

    int pid = 1;
    int pc = 0;

    t_paquete* paquete = crear_paquete(RECIBIR_PROCESO);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &pc,  sizeof(int));

    enviar_paquete(paquete, conexion_cpu);
    eliminar_paquete(paquete);

    char buffer[256];
    recv(conexion_cpu, buffer, sizeof(buffer), 0);
    printf("[KERNEL_MIN] CPU respondió: %s\n", buffer);

    close(conexion_cpu);
    return 0;
}
