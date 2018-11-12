#include "planificador.h"

u_int32_t numero_pid = 1;
u_int32_t procesos_en_memoria = 0;

char* Estados[5] = {"Nuevo", "Listo", "Ejecutando", "Bloqueado", "Finalizado"};

//Hilo planificador largo plazo
void planificador_largo_plazo()
{
    // ejecutar path ejecutar path2 ejecutar path3
    // meto dummy 1, dummy 2, dummy 3 en ready

    while(true)
    {
        if(permite_multiprogramacion() && !list_is_empty(lista_nuevos))
        {
            for(int i = 0; i < list_size(lista_nuevos); i++)
            {
                DTB *dtb_a_cargar = list_get(lista_nuevos, i);
                if(!dummy_creado(dtb_a_cargar)) desbloquear_dtb_dummy(dtb_a_cargar);
            }
            
        }
    }
}

bool dummy_creado(void *_dtb)
{
    DTB *dtb = (DTB *)_dtb;
    int pid = dtb->gdtPID;

    return (encuentra_dtb_asociado_a_pid(lista_listos, pid) != NULL ||
    encuentra_dtb_asociado_a_pid(lista_ejecutando, pid) != NULL);
}

//Hilo planificador corto plazo
void planificador_corto_plazo()
{
    while(true)
    {
        t_cpu* cpu_libre = list_find(lista_cpu, esta_libre_cpu);
        if(cpu_libre != NULL && !list_is_empty(lista_listos))
        {
            printf("lista Cpu: %d", list_size(lista_cpu));
            cpu_libre->estado = CPU_OCUPADA;
            ejecutar_primer_dtb_listo(cpu_libre);
        }
    }
}

void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2) {
    DTB* DTB_a_mover = list_remove(lista1, 0);
    list_add(lista2, DTB_a_mover);

}
//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo(t_cpu* cpu_libre) {
    DTB* DTB_ejecutar = list_remove(lista_listos, 0);
    list_add(lista_ejecutando, DTB_ejecutar);
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
    free(paquete);
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
    list_remove_and_destroy_by_condition(lista_ejecutando, compara_dummy, liberar_dtb);
}

void desbloquear_dtb_dummy(DTB* dtb_nuevo) {
    ArchivoAbierto* escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
    DTB *dtb_dummy = crear_dtb(dtb_nuevo->gdtPID, escriptorio->path, 0);
    list_add(lista_listos, dtb_dummy);
}

void notificar_al_plp(int* pid) {
    DTB* DTB_a_mover = remueve_dtb_asociado_a_pid(lista_nuevos, *pid);
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

DTB_info *info_asociada_a_dtb(t_list *lista, DTB* dtb)
{
    bool compara_con_info(void *info_dtb)
    {
        return coincide_pid_info(dtb->gdtPID, info_dtb);
    }
    return list_find(lista, compara_con_info);
}

DTB  *encuentra_dtb_asociado_a_pid(t_list* lista, int pid)
{
    bool compara_con_dtb(void* DTB) {
        return coincide_pid(pid, DTB);
    }
    return list_find(lista, compara_con_dtb);
}

DTB *remueve_dtb_asociado_a_pid(t_list* lista, int pid)
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

DTB *buscar_dtb_en_todos_lados(int *pid, DTB_info **info_dtb, t_list **lista_actual)
{
    DTB *dtb_encontrado;
    for(int i = 0; i < (list_size(lista_estados)); i++)
    {   
        *lista_actual = list_get(lista_estados, i);
        dtb_encontrado = encuentra_dtb_asociado_a_pid(*lista_actual, *pid);
        if(dtb_encontrado != NULL)
        {
            *info_dtb = info_asociada_a_dtb(lista_info_dtb, dtb_encontrado);
            break;
        }
    }
    return dtb_encontrado;
}

void mostrar_proceso_reducido(void *_dtb)
{
    DTB *dtb = (DTB *)_dtb;
    printf( "PID: %i\n"
            "Archivos Abiertos: %i\n",
            dtb->gdtPID, list_size(dtb->archivosAbiertos));
}

void mostrar_proceso(void *_dtb, void *_info_dtb)
{ 
	DTB* dtb = (DTB *) _dtb;
    DTB_info *info_dtb = (DTB_info *) _info_dtb;
	printf( "Status Proceso:\n"
            "PID: %i\n"
            "Flag inicializacion: %i\n"
            "Program Counter: %i\n"
            "Estado: %s\n"
            // "Tiempo de respuesta: %lu\n"
            "Archivos abiertos: %i\n"
            "Escriptorio:\n",
            dtb->gdtPID, dtb->flagInicializacion, dtb->PC,
            Estados[info_dtb->estado],
            // info_dtb->tiempo_respuesta,
            list_size(dtb->archivosAbiertos));

    list_iterate(dtb->archivosAbiertos, mostrar_archivo);
}

void mostrar_archivo(void *_archivo)
{
    ArchivoAbierto *archivo = (ArchivoAbierto *) _archivo;
    printf( "Directorio: %s\n"
            "Cantidad de lineas: %i\n",
            // Agregar si se agregan campos a ArchivoAbierto
            archivo->path, archivo->cantLineas);
}

//Funciones de Consola
void ejecutar(char* path) {
	DTB* dtb_nuevo = crear_dtb(0, path, 1);
	list_add(lista_nuevos, dtb_nuevo);
}

void status()
{
    for(int i = 0; i < (list_size(lista_estados)); i++)
    {
        t_list *lista_mostrar = list_get(lista_estados, i);
        printf("Cantidad de procesos en Estado %s: %i\n", Estados[i], list_size(lista_mostrar));
        // if(list_size(lista_mostrar) != 0)printf("Informacion asociada a los procesos de la lista:\n");
	    if (list_size(lista_mostrar) != 0) list_iterate(lista_mostrar, mostrar_proceso_reducido);
        // Informacion complementaria de cada cola??
    }
    return;
}

void gdt_status(int* pid)
{
    t_list *lista_actual;
    DTB_info *info_dtb;
    DTB *dtb_status = buscar_dtb_en_todos_lados(pid, &info_dtb, &lista_actual);

    if (dtb_status != NULL) mostrar_proceso(dtb_status, info_dtb);
    else printf("El proceso %i no esta en el sistema\n", *pid);
}

void finalizar(int *pid)
{
    t_list *lista_actual;
    DTB_info *info_dtb;
    DTB *dtb_finalizar = buscar_dtb_en_todos_lados(pid, &info_dtb, &lista_actual);
    
    if (dtb_finalizar != NULL) manejar_finalizar(pid, info_dtb, dtb_finalizar, lista_actual);
    else printf("El proceso %i no esta en el sistema\n", *pid);
}

void manejar_finalizar(int *pid, DTB_info *info_dtb, DTB *dtb_finalizar, t_list *lista_actual)
{
    switch(info_dtb->estado)
    {
        case DTB_EJECUTANDO:
        {
            printf("\n El proceso de pid = %d se encuentra en Ejecucion\n", *pid);
            //Esperar que termine de ejecutar
            list_add(lista_finalizados, dtb_finalizar);
            procesos_en_memoria--;
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
            procesos_en_memoria--;
            break;
        }
        case DTB_NUEVO:
        {
            printf("El proceso %d se encuentra en la cola de nuevos\n", *pid);
            break;
        }
        case DTB_FINALIZADO:
        {
            printf("El proceso %d ya esta en cola de finalizados\n", *pid);
            liberar_dtb(dtb_finalizar);
            break;
        }
        default:
        {
            printf("El proceso %d no esta en ninguna cola\n", *pid);
            break;
        }
    }
}

void metricas();