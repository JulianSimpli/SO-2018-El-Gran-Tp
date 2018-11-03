#include "planificador.h"

//Hilo planificador largo plazo
void planificador_largo_plazo() {
    while(1) {
        if(permite_multiprogramacion() && !lista_vacia(lista_PLP)){
            mover_primero_de_lista1_a_lista2(lista_PLP, lista_listos);
        }
    }
}

//Funciones del planificador largo plazo (las booleanas todas juntas abajo)
void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2) {
    DTB* DTB_a_mover = list_remove(lista1, 0);
    list_add(lista2, DTB_a_mover);
}

//Hilo planificador corto plazo
void planificador_corto_plazo() {
    while(1) {
        t_unCpuSafa* cpu_libre = list_find(lista_cpu, (void*)esta_libre_cpu);
        if(cpu_libre != NULL && !listaVacia(lista_listos)) {
            pthread_mutex_lock(cpu_libre->mutex_estado);
            DTB* DTB_ejecutar = list_remove(lista_listos, 0);
            list_add(lista_ejecutando, DTB_ejecutar);
            ejecutar_primer_DTB_listo(DTB_ejecutar, cpu_libre);
        }
    }
}
//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo(DTB* DTB_ejecutar, t_unCpuSafa* cpu_libre) {
    Paquete* paquete = malloc(sizeof(Paquete));
    int tamanio_DTB = 0;
    paquete->Payload = DTB_serializar(DTB_ejecutar, &tamanio_DTB);
    paquete->header.tamPayload = tamanio_DTB;
    paquete->header.emisor = SAFA;

    int socket_cpu_libre = cpu_libre->socket;

    if(DTB->flagInicializacion = 0) {
        paquete->header.TipoMensaje = ESDTBDUMMY;
        EnviarPaquete(socket_cpu_libre, paquete);
    } else {
        paquete->header.TipoMensaje = ESDTB;
        EnviarPaquete(socket_cpu_libre, paquete);
    }
}

//Funciones asociadas a DTB
DTB* crearDTB(char* path) {

	DTB* DTBNuevo = malloc (sizeof(DTB));
	DTBNuevo->PC = 1;
	DTBNuevo->estado = ESTADO_NUEVO;
	DTBNuevo->flagInicializacion = 1;
	DTBNuevo->pathEscriptorio = string_duplicate(path); //pathEscriptorio es un char*
	DTBNuevo->gdtPID = numeroPID;
	numeroPID++;

	return DTBNuevo;
}

int desbloquear_dtb_dummy(DTB* DTBNuevo) {
    DTB* DTBDummy = malloc (sizeof(DTB));
    dtb_dummy->gdtPID = DTBNuevo->gdtPID;
    dtb_dummy->flagInicializacion = 0;
    dtb_dummy->pathEscriptorio = DTBNuevo->pathEscriptorio;
    
    return list_add(lista_PLP, dtb_dummy);
}

int notificar_al_PLP(PID) {
    DTB* DTB_a_mover = devuelve_DTB_asociado_a_pid(&PID);
    return list_add(lista_PLP, DTB_a_mover);
}

DTB* devuelve_DTB_asociado_a_pid(int* PID, t_list* lista) {
    return list_find(lista, (void*) coincideElPID(PID));
}

//Funciones booleanas
bool coincidePID(DTB* DTB, int* PID) {
    return (DTB->gdtPID == *PID);
}

bool coincide_socket(t_unCpuSafa* cpu, int* socketFD) {
    return cpu->socket == *socketFD;
}

bool permite_multiprogramacion() {
    return procesos_en_memoria < MULTIPROGRAMACION;
}

bool lista_vacia(*t_list lista) {
    return list_size(lista) == 0;
}

bool esta_libre_cpu(void *cpu) {
    return ((t_unCpuSafa*)cpu) -> mutex_estado != 0;
}

bool hay_cpu_libre() {
    return list_any_satisfy(lista_cpu, (void*)esta_libre_la_cpu);
}

//

void desplegarCola(DTB* listaProcesos){ //Modificar y usar con list_iterate y printf

	mostrarProcesos(listaProcesos);
}


void mostrarProcesos(t_list* listaProcesos){
	list_iterate(lista, mostrarProceso);
	printf("\n\n");
}

void mostrarUnProceso(void* process){
	DTB* proceso = (DTB*) process;
	 printf("\ngdtPID: %i\n", proceso->gdtPID);
     printf("flagInicializacion: %i\n", proceso->flagInicializacion);
     //printf("\nfileDir: %s", proceso->????);
     printf("\n");

}

//Funciones de Consola
void ejecutar(char* path) {
	DTB* DTBNuevo = crearDTB(path);
	list_add(listaNuevos, DTBNuevo);
    desbloquear_dtb_dummy(DTBNuevo);
}

void status()     { ////NO Repitas! LLama a una list_iterate

printf("\nCola de Nuevos- cant. de procesos: %d", list_size(listaNuevos));
desplegarCola(listaNuevos);

printf("\nCola de Listos- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(listaListos));
desplegarCola(listaListos);

printf("\nCola de Ejecutados- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(listaEjecutando));
desplegarCola(listaEjecutando);

printf("\nCola de Bloqueados- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(listaBloqueados));
desplegarCola(listaBloqueados);

printf("\nCola de Finalizados- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(listaFinalizados));
desplegarCola(listaFinalizados);

}

void finalizar(int PID){
    ptr3=NULL;

    for(i=0; i<5; i++){
        ptr3 = list_find(stateArray[i], PID);   //Busco a una PCB que tenga ese PID
        if(ptr3 != NULL){
                if(stateArray[i]==Ejecutando){
                    printf("\n El proceso de PID = %d se encuentra en Ejecucion\n", PID);
                    //Esperar que termine de ejecutar
                    //queue_push(Finalizar, ptr3);  //Cuando termine la ejecucion
                    //list_remove_and_destroy_by_condition(, ,); //eliminarlo de la cola
                    printf("\n El proceso de PID %d se ha movido a la cola de Finalizados\n", PID);
                }else{
                    queue_push(Finalizar, ptr3);
                    //list_remove_and_destroy_by_condition(, ,); //eliminarlo de la cola en que se encuentre
                    printf("\n El proceso de PID %d se ha movido a la cola de Finalizados\n", PID);
                }
        }else if(i==4 && ptr3==NULL){
            printf("\n El proceso de PID %d no se encontro en las colas de estados\n", PID);
        }

    }
}

//void metricas()