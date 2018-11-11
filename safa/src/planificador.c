#include "planificador.h"

u_int32_t numero_pid = 1;
u_int32_t procesos_en_memoria = 0;

//Hilo planificador largo plazo
void planificador_largo_plazo() {
    while(1) {
        if(permite_multiprogramacion() && !list_is_empty(lista_nuevos)){
            DTB* dtb_nuevo = list_get(lista_nuevos, 0);
            desbloquear_dtb_dummy(dtb_nuevo);
        }
    }
}

//Hilo planificador corto plazo
void planificador_corto_plazo() {
    while(1) {
        t_cpu* cpu_libre = list_find(lista_cpu, esta_libre_cpu);
        if(cpu_libre != NULL && !list_is_empty(lista_listos)) {
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

    if(DTB_ejecutar->flagInicializacion == 0) {
        cargar_header(&paquete, tamanio_DTB, ESDTBDUMMY, SAFA);
        EnviarPaquete(cpu_libre->socket, paquete);
    } else {
        cargar_header(&paquete, tamanio_DTB, ESDTB, SAFA);
        EnviarPaquete(cpu_libre->socket, paquete);
    }
}

//Funciones asociadas a DTB
DTB *crear_dtb(int pid, char* path, int flag_inicializacion)
{
	DTB* dtb_nuevo = malloc(sizeof(DTB));
    dtb_nuevo->flagInicializacion = flag_inicializacion;
    dtb_nuevo->PC = 0;
    switch(flag_inicializacion) {
        case 0:
        {
            dtb_nuevo->gdtPID = pid;
            break;
        }
        case 1:
        {
            dtb_nuevo->gdtPID = numero_pid;
            numero_pid++;

            DTB_info *info_dtb = malloc(sizeof(DTB_info));
            info_dtb->gdtPID = dtb_nuevo->gdtPID;
            info_dtb->estado = DTB_NUEVO;
            list_add(lista_info_dtb, info_dtb);
            break;
        }
        default:
        {
            printf("flag_inicializacion invalida");
        }
    }

    dtb_nuevo->archivosAbiertos = list_create();
    DTB_agregar_archivo(dtb_nuevo, 0, path);

	return dtb_nuevo;
}

void liberar_dtb(void *dtb)
{
    if (((DTB *)dtb)->flagInicializacion == 1) liberar_info(dtb);
    
    list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
    free(dtb);
}

void liberar_info(void *dtb)
{
    int pid = ((DTB *)dtb)->gdtPID;
    bool coincide_info(void *info_dtb)
    {
        return coincide_pid_info(pid, info_dtb);
    }
    list_remove_and_destroy_by_condition(lista_info_dtb, coincide_info, free);
}

void bloquear_dummy(int *pid)
{
    bool compara_dummy(void *dtb)
    {
        return coincide_pid(*pid, dtb);
    }
    list_remove_and_destroy_by_condition(lista_listos, compara_dummy, liberar_dtb);
}

void desbloquear_dtb_dummy(DTB* dtb_nuevo) {
    DTB* dtb_dummy = malloc (sizeof(DTB));
    ArchivoAbierto* escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
    dtb_dummy = crear_dtb(dtb_nuevo->gdtPID, escriptorio->path, 0);
    list_add(lista_listos, dtb_dummy);
}

void notificar_al_plp(int* pid) {
    DTB* DTB_a_mover = DTB_asociado_a_pid(lista_nuevos, *pid);
    list_add(lista_listos, DTB_a_mover);
    procesos_en_memoria++;
}

bool coincide_pid(int pid, void* DTB_comparar)
{
	return ((DTB *)DTB_comparar)->gdtPID == pid;
}

bool coincide_pid_info(int pid, void *info_dtb)
{
    return ((DTB_info *)info_dtb)->gdtPID == pid;
}

DTB_info *info_asociada_a_pid(t_list *lista, int pid)
{
    bool compara_con_info(void *info_dtb)
    {
        return coincide_pid_info(pid, info_dtb);
    }
    return list_find(lista, compara_con_info);
}

DTB* DTB_asociado_a_pid(t_list* lista, int pid)
{
    bool compara_con_dtb(void* DTB) {
        return coincide_pid(pid, DTB);
    }
    return list_remove_by_condition(lista, compara_con_dtb);
}
// va sin parentesis coincide_pid porque tiene que llamar al puntero a la funcion despues de que hacer list_find

void liberar_cpu(int *socket)
{
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

//

void desplegar_lista(t_list *listaProcesos){ 
	mostrarProcesos(listaProcesos);
}


void mostrarProcesos(t_list* listaProcesos){
	list_iterate(listaProcesos, mostrarUnProceso);
	printf("\n\n");
}

void mostrarUnProceso(void* process){ 
	 DTB* proceso = (DTB*) process;
     ArchivoAbierto archivos_abiertos = DTB_obtener_escriptorio(proceso);
	 printf("\ngdtPID: %i", proceso->gdtPID);
     printf("\nflagInicializacion: %i", proceso->flagInicializacion);
     pritnf("\nProgram Counter: %i", proceso->PC);
     printf("\npath de DTB: %s y cantidad de lineas: %i", archivos_abiertos->path, archivos_abiertos->cantLineas;
     printf("\n");

}

//Funciones de Consola
void ejecutar(char* path) {
	DTB* dtb_nuevo = crear_dtb(0, path, 1);
	list_add(lista_nuevos, dtb_nuevo);
}



DTB* buscar_dtb_por_pid (void* pid_recibido, int index ){ //Usada en status(pid) y finalizar(pid)

    int pid = *pid_recibido;
    int i = index;

    bool compara_con_dtb(void* dtb)
    {
        return coincide_pid(pid, dtb);
    }

    t_list *lista_actual = list_get(lista_estados, i);
    DTB *dtb_encontrado  =  list_find(lista_actual, compara_con_dtb);
    return dtb_encontrado;
}


char nombre_estados[5][12]={"Nuevos","Listos","Ejecutando","Bloqueados", "Finalizados"};

void status() { 

    for(i=0; i<(list_size(lista_estados)); i++){
        t_list *lista_aux = list_get(lista_estados, i);
        printf("\nCantidad de procesos en cola de %s: %i", nombre_estados[i], list_size(lista_aux));
        printf("\nInformacion asociada a los procesos de la lista: \n");
        desplegar_lista(lista_aux);

    }

}

void status(int* pid){
    DTB* dtb_encontrado=NULL;
    int i = 0;
    while(dtb_encontrado == NULL &&  (i < (list_size(lista_estados))) )
    {
        dtb_encontrado = buscar_dtb_por_pid(pid, i);
        i++;
    }
    if(dtb_encontrado != NULL){  
        mostrarUnProceso(dtb_encontrado);
    }else{
        printf("\nEl proceso no se encuentra en el sistema\n");
    }
}



void finalizar(int *pid)
{
    for(int i = 0; i < (list_size(lista_estados)); i++) 
    {   
        if( (buscar_dtb_por_pid(pid, i)) != NULL)
    {
        DTB_info *info_dtb = info_asociada_a_pid(lista_info_dtb, dtb_finalizar->gdtPID);
        manejar_finalizar(pid, info_dtb, dtb_finalizar, lista_actual);
        break;
    }
        
        else if(i == 4 && dtb_finalizar == NULL) printf("El proceso %d no esta en ninguna cola", *pid);
    }
}

void manejar_finalizar(int *pid, DTB_info *info_dtb, DTB *dtb_finalizar, t_list *lista_actual)
{
    switch(info_dtb->estado)
    {
        case DTB_EJECUTANDO:
        {
            printf("\n El proceso de pid = %d se encuentra en Ejecucion\n", *pid);
            //Esperar que termine de ejecutar
            list_add(lista_finalizados, dtb_finalizar);  //Cuando termine la ejecucion
            liberar_dtb(dtb_finalizar);
            printf("\n El proceso de PID %d se ha movido a la cola de Finalizados\n", *pid);
            break;
        }
        case DTB_BLOQUEADO:
        {
            break;
        }
        case DTB_LISTO:
        {
            break;
        }
        case DTB_NUEVO:
        {
            printf("El proceso %d se encuentra en la cola de nuevos", *pid);
            break;
        }
        case DTB_FINALIZADO:
        {
            printf("El proceso %d ya esta en cola de finalizados", *pid);
            liberar_dtb(dtb_finalizar);
            break;
        }
        default:
        {
            printf("El proceso %d no esta en ninguna cola", *pid);
            break;
        }
    }
}

void metricas();