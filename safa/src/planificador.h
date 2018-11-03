#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "../../Bibliotecas/dtb.h"
#include "../../Bibliotecas/helper.h"

typedef enum {
	ESTADO_NUEVO,
	ESTADO_LISTO,
	ESTADO_EJECUTANDO,
	ESTADO_BLOQUEADO,
	ESTADO_FINALIZADO
};

typedef struct {
	int socket;
    pthread_mutex_t mutex_estado;
}__attribute__((packed)) t_unCpuSafa;

t_list* listaNuevos;
t_list* listaListos;
t_list* listaEjecutando;
t_list* listaBloqueados;
t_list* listaFinalizados;

t_list* ptr2;
t_list* ptr3;

u_int32_t i=0, numeroPID = 1, procesosEnMemoria = 0;

//Vector de estructuras para buscar un PID y finalizarlo
DTB* stateArray[5] = {listaNuevos, listaListos, listaEjecutando, listaBloqueados, listaFinalizados};

//Funciones
//Hilo planificador largo plazo
void planificador_largo_plazo();

//Funciones del planificador largo plazo (las booleanas todas juntas abajo)
void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2);

//Hilo planificador corto plazo
void planificador_corto_plazo();

//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo(DTB* DTB_ejecutar, t_unCpuSafa* cpu_libre);

//Funciones asociadas a DTB
DTB* crearDTB(char* path);
int desbloquear_dtb_dummy(DTB* DTBNuevo);
int notificar_al_PLP(PID);
DTB* devuelve_DTB_asociado_a_pid(int* PID, t_list* lista);

//Funciones booleanas
bool coincidePID(DTB* DTB, int* PID);
bool coincide_socket(t_unCpuSafa* cpu, int* socketFD);
bool permite_multiprogramacion();
bool lista_vacia(*t_list lista);
bool esta_libre_cpu(void *cpu);
bool hay_cpu_libre();

//
void desplegarCola(DTB* listaProcesos);
void mostrarProcesos(t_list* listaProcesos);
void mostrarUnProceso(void* process);

//Funciones de Consola
void ejecutar(char* path);
void status();
void finalizar(int PID);
void metricas()

#endif /* PLANIFICADOR_H_ */