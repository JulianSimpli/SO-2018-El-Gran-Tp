#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#define GDT 1
#define DUMMY 0

#include "../../Bibliotecas/dtb.h"
#include "../../Bibliotecas/sockets.h"
#include "../../Bibliotecas/helper.h"

typedef enum {
	DTB_NUEVO, DTB_LISTO, DTB_EJECUTANDO, DTB_BLOQUEADO, DTB_FINALIZADO,
	CPU_LIBRE, CPU_OCUPADA
} Estado;

typedef struct DTB_info {
	u_int32_t gdtPID;
	Estado estado;
	int socket_cpu;
	clock_t *tiempo_ini;
	clock_t *tiempo_fin;
	float tiempo_respuesta;
	bool kill;
	t_list *recursos;
	u_int32_t sentencias_en_nuevo;
	u_int32_t sentencias_al_diego;
	u_int32_t sentencias_hasta_finalizar;
} DTB_info;

typedef struct {
	int socket;
    Estado estado;
}__attribute__((packed)) t_cpu;

typedef struct {
	char *id;
	u_int32_t semaforo;
	t_list *pid_bloqueados;
}__attribute__((packed)) t_recurso;

t_list *lista_cpu;

t_list *lista_nuevos;
t_list *lista_listos;
t_list *lista_ejecutando;
t_list *lista_bloqueados;
t_list *lista_finalizados;

t_list *lista_estados;
t_list *lista_info_dtb;

t_list *lista_recursos_global;

t_log *logger;
t_log* logger_fin;

u_int32_t numero_pid, procesos_en_memoria, procesos_finalizados;
u_int32_t sentencias_globales_del_diego, sentencias_totales;

u_int32_t MULTIPROGRAMACION, RETARDO_PLANIF; //La carga la config y SAFA al inicializarse
char *ALGORITMO_PLANIFICACION;
int socket_diego;
pthread_t hilo_consola, hilo_plp, hilo_pcp;


//Funciones
//Hilo planificador largo plazo
void planificador_largo_plazo();

bool permite_multiprogramacion();
bool dummy_creado(DTB *dtb);

void pasaje_a_ready(u_int32_t pid);
void notificar_al_plp(u_int32_t pid);


//Hilo planificador corto plazo
void planificador_corto_plazo();

void ejecutar_primer_dtb_listo();
void planificar_fifo();
void planificar_rr();
void planificar_vrr();
void planificar_iobound();


//Funciones de DTB
DTB *dtb_crear(u_int32_t pid, char *path, int flag_inicializacion);
DTB_info *info_dtb_crear(u_int32_t pid);

void dtb_liberar(void *dtb);
void info_liberar(void *dtb);

DTB *dtb_actualizar(DTB *dtb, t_list *source, t_list *dest, u_int32_t pc, Estado estado, int socket);
DTB_info* info_dtb_actualizar(Estado estado, int socket, DTB_info *info_dtb);


// Busquedas
DTB *dtb_encuentra(t_list* lista, u_int32_t pid, u_int32_t flag);
DTB *dtb_remueve(t_list* lista, u_int32_t pid, u_int32_t flag);
DTB *dtb_buscar_en_todos_lados(u_int32_t pid, DTB_info **info_dtb, t_list **lista_actual);
bool dtb_coincide_pid(void *dtb, u_int32_t pid, u_int32_t flag);

DTB_info *info_asociada_a_pid(u_int32_t pid);
bool info_coincide_pid(u_int32_t pid, void *info_dtb);


//Funciones de cpu
void liberar_cpu(int socket);
t_cpu* cpu_con_socket(t_list* lista, int socket);
bool coincide_socket(int socket, void* cpu);
bool esta_libre_cpu(void* cpu);
bool hay_cpu_libre();


//Funciones booleanas
bool esta_en_memoria(DTB_info *info_dtb);


//Funciones Dummy
void desbloquear_dummy(DTB* dtb_nuevo);
void bloquear_dummy(t_list* lista, u_int32_t pid);


//Funciones de Consola
//Ejecutar
void ejecutar(char* path);

//Status
void status();
void gdt_status(u_int32_t pid);
void mostrar_proceso(void *_dtb);
void mostrar_archivo(void *_archivo, int index);
void dtb_imprimir_basico(void *_dtb);
void dtb_imprimir_polenta(void *_dtb);

//Finalizar
void finalizar(u_int32_t pid);
void manejar_finalizar(DTB *dtb, u_int32_t pid, DTB_info *info_dtb, t_list *lista_actual);
void manejar_finalizar_bloqueado(DTB* dtb, u_int32_t pid, DTB_info *info_dtb, t_list *lista_actual);
void enviar_finalizar_dam(u_int32_t pid);
void enviar_finalizar_cpu(u_int32_t pid, int socket);
void loggear_finalizacion(DTB* dtb, DTB_info* info_dtb);

//Recursos que usa finalizar
t_recurso *recurso_bloqueando_pid(u_int32_t pid);
void forzar_signal(void *_recurso);
void dtb_signal(t_recurso *recurso);
void recurso_asignar_a_pid(t_recurso *recurso, u_int32_t pid);

//Metricas
void gdt_metricas(u_int32_t pid);
void metricas();
float calcular_sentencias_promedio_diego();
float calcular_sentencias_promedio_hasta_finalizar();
bool ya_finalizo(void *_info_dtb);
float medir_tiempo(int signal, clock_t* tin_rcv, clock_t* tfin_rcv);

#endif /* PLANIFICADOR_H_ */

// Probablemente dejen de usarse
DTB *dtb_reemplazar_de_lista(DTB *dtb_nuevo, t_list *source, t_list *dest, Estado estado);
DTB *mover_dtb_de_lista(DTB *dtb, t_list *source, t_list *dest);
void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2);
void dtb_finalizar_desde(DTB *dtb, t_list *source);
void dummy_finalizar(DTB *dtb);