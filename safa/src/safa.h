#ifndef SAFA_H_
#define SAFA_H_

#include "planificador.h"

//Declaracion de constantes
#define EJECUTAR "ejecutar"
#define STATUS "status"
#define FINALIZAR "finalizar"
#define METRICAS "metricas"

//Declaracion de variables globales
char *IP;
u_int32_t PUERTO, QUANTUM;

t_list *lista_hilos;

bool end;

sem_t mutex_handshake_diego;
sem_t mutex_handshake_cpu;

// Declaracion de funciones
void crear_logger();
void crear_logger_finalizados();
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
void *handshake_cpu_serializar(int *tamanio_payload);
void enviar_handshake_cpu(int socketFD);
void enviar_handshake_diego(int socketFD);
bool verificar_si_murio(DTB *dtb, t_list *lista_origen);

// Metricas
void metricas_actualizar(DTB *dtb, u_int32_t pc);
void actualizar_sentencias_al_diego(DTB *dtb);
void actualizar_sentencias_en_no_finalizados(u_int32_t sentencias_ejecutadas);
void actualizar_sentencias_en_nuevos(u_int32_t sentencias_ejecutadas);
bool es_no_finalizado(void *_info_dtb);
bool es_nuevo(void * _info_dtb);


//Recursos
t_recurso *recurso_crear(char *id_recurso, int valor_inicial);
void recurso_liberar(void *_recurso);

bool coincide_id(void *recurso, char *id);
t_recurso *recurso_encontrar(char* id_recurso);

t_recurso *recurso_recibir(void *payload, int *pid, int *pc);
void recurso_signal(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket);
void recurso_wait(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket);
DTB *dtb_bloquear(u_int32_t pid, u_int32_t pc, int socket);
void avisar_desalojo_a_cpu(u_int32_t pid, u_int32_t pc, int socket);
void seguir_ejecutando(u_int32_t pid, u_int32_t pc, int socket);
void *serializar_pid_y_pc(u_int32_t pid, u_int32_t pc, int *tam_pid_y_pc);

//Estas van a para a otro lado
void *string_serializar(char *string, int *desplazamiento);
char *string_deserializar(void *data, int *desplazamiento);

#endif /* SAFA_H_ */

