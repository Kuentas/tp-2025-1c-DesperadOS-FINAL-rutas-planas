#include "config.h"
#include "commons/config.h"

#include <string.h>
#include <stdlib.h>
#include <commons/log.h>







typedef struct {
    char* ip_memoria;
    char* puerto_memoria; 
    char* puerto_escucha_dispatch; 
    char* puerto_escucha_interrupt; 
    char* puerto_escucha_io; 
    char* algoritmo_corto_plazo; 
    char* algoritmo_ingreso_a_ready; 
    double alfa; 
    int estimacion_inicial;
    int tiempo_suspension; 
    char* log_level; 
} t_config_kernel;

static t_config_kernel* config;

void inicializar_config(char* path_config){
    t_config* config_temporal = config_create(path_config);
    if (config_temporal == NULL){
        fprintf(stderr, "ERROR: Archivo de configuración no encontrado en: \"%s\"\n", path_config);
        return;
    }

    config = malloc(sizeof(t_config_kernel));
    config->ip_memoria = strdup(config_get_string_value(config_temporal, "IP_MEMORIA"));
    config->puerto_memoria = strdup(config_get_string_value(config_temporal, "PUERTO_MEMORIA"));
    config->puerto_escucha_dispatch = strdup(config_get_string_value(config_temporal, "PUERTO_ESCUCHA_DISPATCH"));
    config->puerto_escucha_interrupt = strdup(config_get_string_value(config_temporal, "PUERTO_ESCUCHA_INTERRUPT"));
    config->puerto_escucha_io = strdup(config_get_string_value(config_temporal, "PUERTO_ESCUCHA_IO"));
    config->algoritmo_corto_plazo = strdup(config_get_string_value(config_temporal, "ALGORITMO_CORTO_PLAZO"));
    config->algoritmo_ingreso_a_ready = strdup(config_get_string_value(config_temporal, "ALGORITMO_INGRESO_A_READY"));
    config->alfa = config_get_double_value(config_temporal, "ALFA");
    config->estimacion_inicial = config_get_int_value(config_temporal, "ESTIMACION_INICIAL");
    config->tiempo_suspension = config_get_int_value(config_temporal, "TIEMPO_SUSPENSION");
    config->log_level = strdup(config_get_string_value(config_temporal, "LOG_LEVEL"));

    config_destroy(config_temporal);
}

void liberar_config(){
    free(config->ip_memoria);
    free(config->puerto_memoria);
    free(config->puerto_escucha_dispatch);
    free(config->puerto_escucha_interrupt);
    free(config->puerto_escucha_io);
    free(config->algoritmo_corto_plazo);
    free(config->algoritmo_ingreso_a_ready);
    free(config->log_level);

    free(config);
}

char* obtener_ip_memoria() {
    return config ? config->ip_memoria : NULL;
}

char* obtener_puerto_memoria() {
    return config ? config->puerto_memoria : NULL;
}

char* obtener_puerto_escucha_dispatch(){
    return config ? config->puerto_escucha_dispatch : NULL;
}

char* obtener_puerto_escucha_interrupt(){
    return config ? config->puerto_escucha_interrupt : NULL;
}

char* obtener_puerto_escucha_io() {
    return config ? config->puerto_escucha_io : NULL;
}

char* obtener_algoritmo_corto_plazo() {
    return config ? config->algoritmo_corto_plazo : NULL;
}

char* obtener_algoritmo_ingreso_a_ready() {
    return config ? config->algoritmo_ingreso_a_ready : NULL;
}

double obtener_alfa() {
    return config ? config->alfa : 0.0;
}

int obtener_estimacion_inicial() {
    return config ? config->estimacion_inicial : 0;
}

int obtener_tiempo_suspension() {
    return config ? config->tiempo_suspension : 0; 
}

char* obtener_log_level() {
    return config ? config->log_level : NULL;
}