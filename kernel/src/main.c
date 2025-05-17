#include <stdlib.h>
#include <string.h>
#include "commons/log.h"
#include "config.h"
#include "cpu.h"
#include "memoria.h"
#include "io.h"
#include "planificador_largo_plazo.h"

t_log* logger;
int main(int argc, char* argv[]) {
    inicializar_config("config/kernel.config");
    logger = log_create("logs/kernel.log", "kernel", true, log_level_from_string(obtener_log_level()));

    if (argc < 3) {
        log_error(logger, "Faltan parametros al momento de instanciar.");
        exit(EXIT_FAILURE);
    }
    
    char* path_process = strdup(argv[1]);
    int size_process = atoi(argv[2]);
    
    inicializar_io();
    inicializar_cpu();
    inicializar_planificador_largo_plazo(path_process, size_process);

    return 0;
}