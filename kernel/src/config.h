#ifndef CONFIG_H
#define CONFIH_H

/**
 * @brief Crea y carga una configuración del kernel desde un archivo
 * 
 * @param path_config Ruta del archivo de configuración (.config)
 * @return t_config_kernel* Estructura con los parámetros cargados
 * @return NULL Si falla la carga del archivo o asignación de memoria
 * 
 * @details 
 * - Asigna memoria para la estructura t_config_kernel
 * - Carga todos los parámetros del archivo como strings duplicados (strdup)
 * - Los valores numéricos se cargan directamente
 * - Libera la estructura config original después de cargar los valores
 * - EL CALLER DEBE LIBERAR LA MEMORIA con liberar_config()
 * 
 * @warning Los strings devueltos son copias independientes en memoria
 */
void inicializar_config(char* path_config);

/**
 * @brief Libera todos los recursos de una configuración del kernel
 * 
 * @param config Estructura de configuración a liberar
 * 
 * @details
 * - Libera todos los strings almacenados en la estructura
 * - Libera la memoria de la estructura principal
 * - Seguro para llamar con NULL (no-op)
 * 
 * @post Después de llamar a esta función, el puntero config queda inválido
 */
void liberar_config();

/**
 * @brief Obtiene la dirección IP de la memoria.
 * 
 * @return char* Dirección IP de la memoria o NULL si config es NULL.
 */
char* obtener_ip_memoria();

/**
 * @brief Obtiene el puerto de la memoria.
 * 
 * @return char* Puerto de la memoria o NULL si config es NULL.
 */
char* obtener_puerto_memoria();

/**
 * @brief Obtiene el puerto de escucha para el dispatch.
 * 
 * @return char* Puerto de escucha para el dispatch o NULL si config es NULL.
 */
char* obtener_puerto_escucha_dispatch();

/**
 * @brief Obtiene el puerto de escucha para las interrupciones.
 * 
 * @return char* Puerto de escucha para las interrupciones o NULL si config es NULL.
 */
char* obtener_puerto_escucha_interrupt();

/**
 * @brief Obtiene el puerto de escucha para operaciones de I/O.
 * 
 * @return char* Puerto de escucha para I/O o NULL si config es NULL.
 */
char* obtener_puerto_escucha_io();

/**
 * @brief Obtiene el algoritmo de planificación para el el planificador de corto plazo.
 * 
 * @return char* Algoritmo de planificación o NULL si config es NULL.
 */
char* obtener_algoritmo_corto_plazo();

/**
 * @brief Obtiene el algoritmo de la cola ready.
 * 
 * @return char* Algoritmo de la cola `new` o NULL si config es NULL.
 */
char* obtener_algoritmo_ingreso_a_ready();

/**
 * @brief Obtiene el valor alfa para el calculo de la estimacion.
 * 
 * @return double Valor alfa o 0.0 si config es NULL.
 */
double obtener_alfa();

/**
 * @brief Obtiene la estimacion inicial.
 * 
 * @return int Valor de la estimacion inicial o 0 si config es NULL.
 */
int obtener_estimacion_inicial();

/**
 * @brief Obtiene el tiempo de suspensión.
 * 
 * @return int Tiempo de suspensión o 0 si config es NULL.
 */
int obtener_tiempo_suspension();

/**
 * @brief Obtiene el nivel de log.
 * 
 * @return char* Nivel de log o NULL si config es NULL.
 */
char* obtener_log_level();


#endif