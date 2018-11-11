#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

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
	time_t tiempo_respuesta;
} DTB_info;

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
t_list *lista_estados;
t_list *vector_estados[5];
t_list *lista_info_dtb;

t_list* ptr2;
t_list* ptr3;

u_int32_t numero_pid;
u_int32_t procesos_en_memoria;
int MULTIPROGRAMACION; //La carga la config y SAFA al inicializarse

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
DTB *crear_dtb(int pid, char *path, int flag_inicializacion);
void liberar_dtb(void *dtb);
void liberar_info(void *dtb);
void desbloquear_dtb_dummy(DTB* dtb_nuevo);
void bloquear_dummy(int *pid);
void notificar_al_plp(int *pid);
DTB *DTB_asociado_a_pid(t_list* lista, int pid);
bool coincide_pid(int pid, void* DTB);
DTB_info *info_asociada_a_pid(t_list *lista, int pid);
bool coincide_pid_info(int pid, void *info_dtb);

//Funciones de cpu
void liberar_cpu(int *socket);
t_cpu* cpu_con_socket(t_list* lista, int* socket);
bool coincide_socket(int* socket, void* cpu);
bool esta_libre_cpu(void* cpu);
t_cpu *cpu_libre();

//Funciones booleanas
bool permite_multiprogramacion();

//
void desplegarCola(DTB* listaProcesos);
void mostrarProcesos(t_list* listaProcesos);
void mostrarUnProceso(void* process);

//Funciones de Consola
void ejecutar(char* path);
void status();
void status(int* pid);
void finalizar(int *pid);
void manejar_finalizar(int *pid, DTB_info *info_dtb, DTB *dtb_finalizar, t_list *lista_actual);
void metricas();
DTB* buscar_dtb_por_pid (void* pid_recibido, int index );


#endif /* PLANIFICADOR_H_ */