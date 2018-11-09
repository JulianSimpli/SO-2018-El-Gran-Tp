#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "../../Bibliotecas/dtb.h"
#include "../../Bibliotecas/sockets.h"
#include "../../Bibliotecas/helper.h"

typedef enum {
	DTB_NUEVO, DTB_LISTO, DTB_EJECUTANDO, DTB_BLOQUEADO, DTB_FINALIZADO,
	CPU_LIBRE, CPU_OCUPADA
} Estado;

typedef struct DTB_local {
	u_int32_t pid;
	Estado estado;
	time_t tiempo_respuesta;
} DTB_local;

typedef struct {
	int socket;
    Estado estado;
}__attribute__((packed)) t_cpu;

t_list *lista_cpu;
t_list *lista_nuevos;
t_list *lista_listos;
t_list *lista_ejecutando;
t_list *lista_bloqueados;
t_list *lista_finalizados;

t_list* ptr2;
t_list* ptr3;

u_int32_t numero_pid;
u_int32_t procesos_en_memoria;
int MULTIPROGRAMACION; //La carga la config y SAFA al inicializarse

//Vector de estructuras para buscar un PID y finalizarlo
t_list *stateArray[5];

//Funciones
//Hilo planificador largo plazo
void planificador_largo_plazo();

//Funciones del planificador largo plazo (las booleanas todas juntas abajo)
void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2);

//Hilo planificador corto plazo
void planificador_corto_plazo();

//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo(DTB* DTB_ejecutar, t_cpu* cpu_libre);

//Funciones de DTB
DTB *crear_dtb(int pid_asociado, char *path, int flag_inicializacion);
void liberar_dtb(void *dtb);
void desbloquear_dtb_dummy(DTB* dtb_nuevo);
void notificar_al_plp(int *pid);
DTB *DTB_asociado_a_pid_de_lista(t_list* lista, int* pid);
bool coincide_pid(int* pid, void* DTB);

//Funciones de cpu
void liberar_cpu(int *socket);
t_cpu* cpu_con_socket(t_list* lista, int* socket);
bool coincide_socket(int* socket, void* cpu);
bool esta_libre_cpu(void* cpu);
t_cpu *cpu_libre();

//Funciones booleanas
bool permite_multiprogramacion();
bool lista_vacia(t_list* lista);

//
void desplegarCola(DTB* listaProcesos);
void mostrarProcesos(t_list* listaProcesos);
void mostrarUnProceso(void* process);

//Funciones de Consola
void ejecutar(char* path);
void status();
void finalizar(int PID);
void metricas();

#endif /* PLANIFICADOR_H_ */