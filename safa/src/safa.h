#ifndef SAFA_H_
#define SAFA_H_

#include "planificador.h"
#include <sys/inotify.h>

//Declaracion de constantes
#define EJECUTAR "ejecutar"
#define STATUS "status"
#define FINALIZAR "finalizar"
#define METRICAS "metricas"
#define LIBERAR "liberar"

#define EVENT_SIZE (sizeof(struct inotify_event) + 256)
#define BUF_INOTIFY_LEN (1024 * EVENT_SIZE)


//Declaracion de variables globales
char *IP;
u_int32_t PUERTO, QUANTUM;

t_list *lista_hilos;

bool end;

pthread_t hilo_event_watcher;

sem_t mutex_handshake_diego;
sem_t mutex_handshake_cpu;

// Declaracion de funciones
void crear_loggers();
void inicializar_variables();
void crear_listas();
void llenar_lista_estados();
void obtener_valores_archivo_configuracion();
void imprimir_archivo_configuracion();
void consola();
void parseo_consola(char* operacion, char* primerParametro);
void accion(void* socket);
void manejar_paquetes_diego(Paquete *paquete, int socketFD);
void manejar_paquetes_CPU(Paquete *paquete, int socketFD);
void *config_cpu_serializar(int *tamanio_payload);
void enviar_handshake_cpu(int socketFD);
void enviar_handshake_diego(int socketFD);
bool verificar_si_murio(DTB *dtb, t_list *lista_origen, u_int32_t pc);

// Metricas
void metricas_actualizar(DTB *dtb, u_int32_t pc);
void actualizar_sentencias_al_diego(DTB *dtb);
void actualizar_sentencias_en_no_finalizados(u_int32_t sentencias_ejecutadas);
void actualizar_sentencias_en_nuevos(u_int32_t sentencias_ejecutadas);
bool es_no_finalizado(void *_info_dtb);
bool es_nuevo(void * _info_dtb);


//Recursos
t_recurso *recurso_recibir(void *payload, int *pid, int *pc, Tipo senial);
void recurso_signal(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket);
void recurso_wait(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket);
void avisar_desalojo_a_cpu(int socket);
void seguir_ejecutando(int socket);

// Event watcher (inotify)
void event_watcher();
void enviar_valores_config(void *_cpu);

// Desconexiones
void manejar_desconexion(int socket);
void manejar_desconexion_cpu(int socket);

#endif /* SAFA_H_ */

