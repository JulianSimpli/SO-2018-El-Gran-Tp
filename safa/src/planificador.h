#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "../../Bibliotecas/dtb.h"
#include "../../Bibliotecas/sockets.h"
#include <stdio.h>

typedef enum {
	ESTADO_NUEVO,
	ESTADO_LISTO,
	ESTADO_EJECUTANDO,
	ESTADO_BLOQUEADO,
	ESTADO_FINALIZADO
} Estado;

typedef struct {
	int socket;
    pthread_mutex_t mutex_estado;
}__attribute__((packed)) t_cpu;

t_list *lista_cpu;
t_list *lista_nuevos;
t_list *lista_listos;
t_list *lista_ejecutando;
t_list *lista_bloqueados;
t_list *lista_finalizados;
t_list *lista_PLP;

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

//Funciones asociadas a DTB
DTB* crearDTB(char* path);
int desbloquear_dtb_dummy(DTB* DTBNuevo);
int notificar_al_PLP(t_list* lista, int *pid);

//Funciones de listas
DTB* devuelve_DTB_asociado_a_pid_de_lista(t_list* lista, int* pid);
t_cpu* devuelve_cpu_asociada_a_socket_de_lista(t_list* lista, int* socket);

//Funciones booleanas
bool coincide_pid(int* pid, void* DTB);
bool coincide_socket(int* socket, void* cpu);
bool permite_multiprogramacion();
bool lista_vacia(t_list* lista);
bool esta_libre_cpu(t_cpu* cpu);
bool hay_cpu_libre();

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
