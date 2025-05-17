#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "utils/conexiones.h"
#include "commons/log.h"
#include "commons/config.h"

t_log* logger;

int main(int argc, char* argv[]) {    
    t_config* config = config_create("config/io.config");
    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    char* log_level = config_get_string_value(config, "LOG_LEVEL");

    logger = log_create("logs/io.log", "io", true, log_level_from_string(log_level));
    
    if (argc == 1) {
        log_error(logger, "No se ha introducido el nombre del diapositivo IO.");
        return EXIT_FAILURE;
    }
    
    int fd_kernel = conectar_a_servidor(ip_kernel, puerto_kernel);
    if (fd_kernel < 0) {
        log_error(logger, "No se ha podido establecer conexion con el modulo Kernel.");
        return EXIT_FAILURE;
    }

    log_debug(logger, "Conexion establecida con el modulo Kernel.");

    char* nombre = argv[1];
    int size = strlen(nombre)+1;
    
    send(fd_kernel, &size, sizeof(int), 0);
    send(fd_kernel, nombre, size , 0);

    int tiempo;
    int bytes_recibidos = recv(fd_kernel, &tiempo, sizeof(int), 0); // tambien debe recibir el PID
    while (bytes_recibidos > 0){
        log_info(logger, "## PID: <PID> - Inicio de IO - Tiempo: %i.", tiempo);
        usleep(tiempo);
        log_info(logger, "## PID: <PID> - Fin de IO.");
        // send(fd_kernel, ) 
        bytes_recibidos = recv(fd_kernel, &tiempo, sizeof(int), 0);
    }
    
    // Al finalizar deberá informar al Kernel que finalizó la solicitud de I/O y quedará a la espera de la siguiente petición.
    // Al momento de recibir una petición del Kernel, el módulo deberá iniciar un usleep por el tiempo indicado en la request.

    return 0;
}
