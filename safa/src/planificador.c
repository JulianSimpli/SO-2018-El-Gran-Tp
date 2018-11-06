#include "planificador.h"

u_int32_t numero_pid = 1;
u_int32_t procesos_en_memoria = 0;

//Hilo planificador largo plazo
void planificador_largo_plazo() {
    while(1) {
        if(permite_multiprogramacion() && !lista_vacia(lista_plp)){
            mover_primero_de_lista1_a_lista2(lista_plp, lista_listos);
            procesos_en_memoria++;
        }
    }
}

//Hilo planificador corto plazo
void planificador_corto_plazo() {
    while(1) {
        t_cpu* cpu_libre = list_find(lista_cpu, esta_libre_cpu);
        if(cpu_libre != NULL && !lista_vacia(lista_listos)) {
            cpu_libre->estado = CPU_OCUPADA;
            DTB* DTB_ejecutar = list_remove(lista_listos, 0);
            list_add(lista_ejecutando, DTB_ejecutar);
            ejecutar_primer_dtb_listo(DTB_ejecutar, cpu_libre);
        }
    }
}

void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2) {
    DTB* DTB_a_mover = list_remove(lista1, 0);
    list_add(lista2, DTB_a_mover);

}
//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo(DTB* DTB_ejecutar, t_cpu* cpu_libre) {
    Paquete* paquete = malloc(sizeof(Paquete));
    int tamanio_DTB = 0;
    paquete->Payload = DTB_serializar(DTB_ejecutar, &tamanio_DTB);
    paquete->header.tamPayload = tamanio_DTB;
    paquete->header.emisor = SAFA;

    if(DTB_ejecutar->flagInicializacion == 0) {
        paquete->header.tipoMensaje = ESDTBDUMMY;
        EnviarPaquete(cpu_libre->socket, paquete);
    } else {
        paquete->header.tipoMensaje = ESDTB;
        EnviarPaquete(cpu_libre->socket, paquete);
    }
}

//Funciones asociadas a DTB
DTB *crear_dtb(int pid_asociado, char* path, int flag_inicializacion) {
	DTB* dtb_nuevo = malloc(sizeof(DTB));
    dtb_nuevo->flagInicializacion = flag_inicializacion;
    dtb_nuevo->PC = 1;
    switch(flag_inicializacion) {
        case 0: {
            dtb_nuevo->gdtPID = pid_asociado;
            break;
        }
        case 1: {
            dtb_nuevo->gdtPID = numero_pid;
            numero_pid++;
            break;
        }
        default: {
            printf("flag_inicializacion invalida");
        }
    }

    dtb_nuevo->archivosAbiertos = list_create();
    DTB_agregar_archivo(dtb_nuevo, 0, path);

	return dtb_nuevo;
}

void liberar_dtb(void *dtb) {
    list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
    free(dtb);
}

void desbloquear_dtb_dummy(DTB* dtb_nuevo) {
    DTB* dtb_dummy = malloc (sizeof(DTB));
    ArchivoAbierto* escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
    dtb_dummy = crear_dtb(dtb_nuevo->gdtPID, escriptorio->path, 0);
    list_add(lista_plp, dtb_dummy);
}

void notificar_al_PLP(t_list* lista, int* pid) {
    DTB* DTB_a_mover = devuelve_DTB_asociado_a_pid_de_lista(lista, pid);
    list_add(lista_plp, DTB_a_mover);
}

bool coincide_pid(int* pid, void* DTB_comparar) {
	return ((DTB*)DTB_comparar)->gdtPID == *pid;
}

DTB* devuelve_DTB_asociado_a_pid_de_lista(t_list* lista, int* pid) {
    bool compara_con_DTB(void* DTB) {
        return coincide_pid(pid, DTB);
    }
    return list_remove_by_condition(lista, compara_con_DTB);
}
// va sin parentesis coincide_pid porque tiene que llamar al puntero a la funcion despues de que hacer list_find

void liberar_cpu(t_list *lista, int *socket) {
    t_cpu* cpu_actual = cpu_con_socket(lista_cpu, socket);
    cpu_actual->estado = CPU_LIBRE;
}

bool coincide_socket(int* socket, void* cpu) {
	return ((t_cpu*)cpu)->socket == *socket;
}

t_cpu* cpu_con_socket(t_list* lista, int* socket) {
    bool compara_cpu(void* cpu) {
        return coincide_socket(socket, cpu);
    }
    return list_find(lista, compara_cpu);
}

bool esta_libre_cpu(void* cpu) {
    return ((t_cpu*)cpu)->estado == CPU_LIBRE;
}

//Funciones booleanas

bool permite_multiprogramacion() {
    return procesos_en_memoria < MULTIPROGRAMACION;
}

bool lista_vacia(t_list* lista) {
    return list_size(lista) == 0;
}

//

void desplegar_lista(t_list *listaProcesos){ //Modificar y usar con list_iterate y printf

	mostrarProcesos(listaProcesos);
}


void mostrarProcesos(t_list* listaProcesos){
	list_iterate(listaProcesos, mostrarUnProceso);
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
	DTB* dtb_nuevo = crear_dtb(0, path, 1);
	list_add(lista_nuevos, dtb_nuevo);
    desbloquear_dtb_dummy(dtb_nuevo);
}

void status() { ////NO Repitas! LLama a una list_iterate

printf("\nCola de Nuevos- cant. de procesos: %d", list_size(lista_nuevos));
desplegar_lista(lista_nuevos);

printf("\nCola de Listos- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(lista_listos));
desplegar_lista(lista_listos);

printf("\nCola de Ejecutados- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(lista_ejecutando));
desplegar_lista(lista_ejecutando);

printf("\nCola de Bloqueados- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(lista_bloqueados));
desplegar_lista(lista_bloqueados);

printf("\nCola de Finalizados- cant. de procesos: %d\nInfo. procesos en esta cola:\n", list_size(lista_finalizados));
desplegar_lista(lista_finalizados);

}
/*
void finalizar(int PID){
    ptr3=NULL;

    for(i=0; i<5; i++){
        ptr3 = list_find(stateArray[i], PID);   //Busco a una PCB que tenga ese PID
        if(ptr3 != NULL){
                if(stateArray[i]== ESTADO_EJECUTANDO){
                    printf("\n El proceso de PID = %d se encuentra en Ejecucion\n", PID);
                    //Esperar que termine de ejecutar
                    //queue_push(Finalizar, ptr3);  //Cuando termine la ejecucion
                    //list_remove_and_destroy_by_condition(, ,); //eliminarlo de la cola
                    printf("\n El proceso de PID %d se ha movido a la cola de Finalizados\n", PID);
                }else{
                    queue_push(ptr3, (void*)ESTADO_FINALIZADO);
                    //list_remove_and_destroy_by_condition(, ,); //eliminarlo de la cola en que se encuentre
                    printf("\n El proceso de PID %d se ha movido a la cola de Finalizados\n", PID);
                }
        }else if(i==4 && ptr3==NULL){
            printf("\n El proceso de PID %d no se encontro en las colas de estados\n", PID);
        }

    }
}

void metricas()

*/