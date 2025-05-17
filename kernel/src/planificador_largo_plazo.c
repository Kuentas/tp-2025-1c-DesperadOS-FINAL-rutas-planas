#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "planificador_largo_plazo.h"
#include "commons/temporal.h"
#include "commons/collections/list.h"
#include "commons/string.h"
#include "commons/log.h"
#include "config.h"
#include "memoria.h"
#include "utils/serializacion.h"


typedef enum {
    NEW ,
    READY,
    EXEC,
    BLOCKED,
    EXIT,
    SUSP_READY,
    SUSP_BLOQUED,
    NUM_ESTADOS,
} t_estado;

typedef struct {
    int pid;
    int pc;
    int me[NUM_ESTADOS];
    t_temporal* mt[NUM_ESTADOS];
    char* path_process;
    int size_process;
} t_pcb;


static t_list* list_new;

extern t_log* logger;
static int pid = 0;
static char* algoritmo;

static pthread_mutex_t mutex_pid = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_list_new = PTHREAD_MUTEX_INITIALIZER;


static t_pcb* _crear_pcb(char* path_file, int size_file);
static bool _tiene_mayor_tamanio_que(void* a, void* b);
static void _print_list_new(void* ptr);
static void _agregar_proceso_a_new(t_pcb* pcb);
static void* _planificador_largo_plazo(void* arg);


void inicializar_planificador_largo_plazo(char* path_process, int size_process){
    algoritmo = obtener_algoritmo_ingreso_a_ready();
    list_new = list_create();
    
    pthread_t pthread;
    pthread_create(&pthread, NULL, _planificador_largo_plazo, NULL);
    pthread_detach(pthread);
    
    crear_nuevo_proceso(path_process, size_process);
}

//la idea es que si no hay procesos en NEW y no tenes ningun proceso en SUSP. READY, lo intentes crear de una.
//Ahora si tenes procesos en NEW (y no tenes nada en SUSP. READY) el intentar crear el proceso va a depender del algoritmo.
//Si el algoritmo es FIFO, no lo intentas crear porque ya tenes procesos antes.
//Si el algoritmo es mas chico primero, te fijas si es mas chico que el primero de la lista y si es asi, lo intentas crear.

static void* _planificador_largo_plazo(void* arg){
    while (true){
        
    
    
    }
    
    return NULL;
}

void crear_nuevo_proceso(char* path_process, int size_process){
    t_pcb* pcb = _crear_pcb(path_process, size_process);
    _agregar_proceso_a_new(pcb);
}

static t_pcb* _crear_pcb(char* path_process, int size_process){
    t_pcb *pcb = malloc(sizeof(*pcb)); 
    
    pthread_mutex_lock(&mutex_pid);
    pcb->pid = pid++;
    pthread_mutex_unlock(&mutex_pid);
    
    pcb->pc = 0;
    
    pcb->me[NEW] = 0;
    pcb->me[READY] = 0;
    pcb->me[EXEC] = 0;
    pcb->me[BLOCKED] = 0;
    pcb->me[SUSP_READY] = 0;
    pcb->me[SUSP_BLOQUED] = 0;

    pcb->mt[NEW] = NULL;
    pcb->mt[READY] = NULL;
    pcb->mt[EXEC] = NULL;
    pcb->mt[BLOCKED] = NULL;
    pcb->mt[SUSP_READY] = NULL;
    pcb->mt[SUSP_BLOQUED] = NULL;

    pcb->path_process = string_duplicate(path_process);
    pcb->size_process = size_process;

    return pcb;
}

static void _agregar_proceso_a_new(t_pcb* pcb){
    if (string_equals_ignore_case(algoritmo, "FIFO")){
        pthread_mutex_lock(&mutex_list_new);
        list_add(list_new, pcb);
        pthread_mutex_unlock(&mutex_list_new);
    } else if (string_equals_ignore_case(algoritmo, "PMCP")) {
        pthread_mutex_lock(&mutex_list_new);
        list_add_sorted(list_new, pcb, _tiene_mayor_tamanio_que);
        pthread_mutex_unlock(&mutex_list_new);
    }
    log_info(logger, "## (%d) Se crea el proceso - Estado: NEW", pcb->pid);
}

static bool _tiene_mayor_tamanio_que(void* a, void* b) {
    t_pcb* pcb_a = (t_pcb*) a;
    t_pcb* pcb_b = (t_pcb*) b;
    return pcb_a->size_process < pcb_b->size_process;
}

static void _print_list_new(void* ptr){
    t_pcb* pcb = (t_pcb*) ptr;
    printf("path_process: %s size process: %d\n", pcb->path_process ,pcb->size_process);
}