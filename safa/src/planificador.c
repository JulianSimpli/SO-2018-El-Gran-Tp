#include "planificador.h"

u_int32_t numero_pid = 0;
u_int32_t procesos_en_memoria = 0;
u_int32_t procesos_finalizados = 0;

char* Estados[5] = {"Nuevo", "Listo", "Ejecutando", "Bloqueado", "Finalizado"};

//Hilo planificador largo plazo
void planificador_largo_plazo()
{
    // ejecutar path ejecutar path2 ejecutar path3
    // meto dummy 1, dummy 2, dummy 3 en ready

    while(true)
    {
        //wait(sem_plp)/ el signal lo hacen ejecutar y mensaje de finalizar.
        // pensar si primitiva finalizar hace signal.
        if(permite_multiprogramacion() && !list_is_empty(lista_nuevos))
        {
            for(int i = 0; i < list_size(lista_nuevos); i++)
            {
                DTB *dtb_cargar = list_get(lista_nuevos, i);
                if(!dummy_creado(dtb_cargar) && permite_multiprogramacion())
                    desbloquear_dummy(dtb_cargar);
            }
        }
        sleep(1);
    }
}

//Hilo planificador corto plazo
void planificador_corto_plazo()
{
    while(true)
    {
        if(hay_cpu_libre() && !list_is_empty(lista_listos))
        {
            if(!strcmp(ALGORITMO_PLANIFICACION, "FIFO")) planificar_fifo();
            else if(!strcmp(ALGORITMO_PLANIFICACION, "RR")) planificar_rr();
            else if(!strcmp(ALGORITMO_PLANIFICACION, "VRR")) planificar_vrr();
            else if(!strcmp(ALGORITMO_PLANIFICACION, "IOBF")) planificar_iobound();
            else
            {
                printf("No se conoce el algoritmo. Cambielo desde SAFA.config\n");
                sleep(10);
            }
        }
        sleep(1);
    }
}

void planificar_fifo()
{
    ejecutar_primer_dtb_listo();
}
void planificar_rr()
{
    // rearma cola listo y luego 
    ejecutar_primer_dtb_listo();
}
void planificar_vrr()
{
    // rearma cola listo y luego 
    ejecutar_primer_dtb_listo();
}
void planificar_iobound()
{
        // rearma cola listo y luego 
    ejecutar_primer_dtb_listo();
}

//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo() {
	DTB_info *info_dtb;
    t_cpu* cpu_libre = list_find(lista_cpu, esta_libre_cpu);
    cpu_libre->estado = CPU_OCUPADA;

    DTB *dtb_exec = list_remove(lista_listos, 0);
    dtb_actualizar(dtb_exec, lista_listos, lista_ejecutando, dtb_exec->PC, DTB_EJECUTANDO, cpu_libre->socket);
    
    Paquete* paquete = malloc(sizeof(Paquete));
    int tamanio_DTB = 0;
    paquete->Payload = DTB_serializar(dtb_exec, &tamanio_DTB);

    switch(dtb_exec->flagInicializacion)
    {
        case DUMMY:
        {
            paquete->header = cargar_header(tamanio_DTB, ESDTBDUMMY, SAFA);
            EnviarPaquete(cpu_libre->socket, paquete);
            printf("Dummy %d ejecutando en cpu %d\n", dtb_exec->gdtPID, cpu_libre->socket);
            break;
        }
        case GDT:
        { 
            paquete->header = cargar_header(tamanio_DTB, ESDTB, SAFA);
            EnviarPaquete(cpu_libre->socket, paquete);
            printf("GDT %d ejecutando en cpu %d\n", dtb_exec->gdtPID, cpu_libre->socket);
            break;
        }
    }
    free(paquete);
}

//Funciones asociadas a DTB
DTB *dtb_crear(u_int32_t pid, char* path, int flag_inicializacion)
{
	DTB *dtb_nuevo = malloc(sizeof(DTB));
    dtb_nuevo->flagInicializacion = flag_inicializacion;
    dtb_nuevo->PC = 1;
    dtb_nuevo->archivosAbiertos = list_create();
    DTB_agregar_archivo(dtb_nuevo, 0, path);

    switch(flag_inicializacion)
    {
        case DUMMY:
        {
            dtb_nuevo->gdtPID = pid;
            list_add(lista_listos, dtb_nuevo);
            break;
        }
        case GDT:
        {
            dtb_nuevo->gdtPID = ++numero_pid;
            info_dtb_crear(dtb_nuevo->gdtPID);
            list_add(lista_nuevos, dtb_nuevo);
            break;
        }
        default:
            printf("flag_inicializacion invalida");
    }

	return dtb_nuevo;
}

DTB_info *info_dtb_crear(u_int32_t pid)
{
    DTB_info *info_dtb = malloc(sizeof(DTB_info));
    info_dtb->gdtPID = pid;
    info_dtb->estado = DTB_NUEVO;
    info_dtb->socket_cpu = 0;
    info_dtb->tiempo_respuesta = 0;
    info_dtb->kill = false;
    info_dtb->recursos = list_create();
    //*info_dtb->tiempo_ini
    //*info_dtb->tiempo_fin
    list_add(lista_info_dtb, info_dtb);

    return info_dtb;
}

DTB *dtb_actualizar(DTB *dtb, t_list *source, t_list *dest, u_int32_t pc, Estado estado, int socket)
{
    if(dtb_encuentra(source, dtb->gdtPID, dtb->flagInicializacion) != NULL)
        dtb_remueve(source, dtb->gdtPID, dtb->flagInicializacion);

    list_add(dest, dtb);
    dtb->PC = pc;

    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
    info_dtb_actualizar(estado, socket, info_dtb);
    switch(estado)
    {
        case DTB_FINALIZADO: procesos_finalizados++;
        case DTB_LISTO: procesos_en_memoria++;
    }

    return dtb;
}

DTB_info* info_dtb_actualizar(Estado estado, int socket, DTB_info *info_dtb)
{
	info_dtb->socket_cpu = socket;
	info_dtb->estado = estado;
    return info_dtb;
}

void dtb_liberar(void *dtb)
{
    if (((DTB *)dtb)->flagInicializacion == GDT)
        info_liberar(dtb);
    list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
    free(dtb);
}

void info_liberar(void *dtb)
{
    u_int32_t pid = ((DTB *)dtb)->gdtPID;
    bool coincide_info(void *info_dtb)
    {
        return info_coincide_pid(pid, info_dtb);
    }
    list_remove_and_destroy_by_condition(lista_info_dtb, coincide_info, free);
}

void bloquear_dummy(t_list *lista, u_int32_t pid)
{
    bool compara_dummy(void *dtb)
    {
        return dtb_coincide_pid(dtb, pid, DUMMY);
    }
    list_remove_and_destroy_by_condition(lista, compara_dummy, dtb_liberar);
}

void desbloquear_dummy(DTB* dtb_nuevo)
{
    ArchivoAbierto* escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
    DTB *dtb_dummy = dtb_crear(dtb_nuevo->gdtPID, escriptorio->path, DUMMY);
}

void notificar_al_plp(u_int32_t pid)
{
    DTB* DTB_a_mover = dtb_remueve(lista_nuevos, pid, 1);
    list_add(lista_listos, DTB_a_mover);
    procesos_en_memoria++;
}

bool dtb_coincide_pid(void* dtb, u_int32_t pid, u_int32_t flag)
{
	return ((DTB *)dtb)->gdtPID == pid && ((DTB *)dtb)->flagInicializacion == flag;
}

bool info_coincide_pid(u_int32_t pid, void *info_dtb)
{
    return ((DTB_info *)info_dtb)->gdtPID == pid;
}

DTB_info *info_asociada_a_pid(u_int32_t pid)   //No hace lo mismo que la de arriba? COMPLETAR
{
    bool compara_con_info(void *info_dtb)
    {
        return info_coincide_pid(pid, info_dtb);
    }
    return list_find(lista_info_dtb, compara_con_info);
}

DTB *dtb_encuentra(t_list* lista, u_int32_t pid, u_int32_t flag)
{
    bool compara_con_dtb(void* dtb) {
        return dtb_coincide_pid(dtb, pid, flag);
    }
    return list_find(lista, compara_con_dtb);
}

DTB *dtb_remueve(t_list* lista, u_int32_t pid, u_int32_t flag)
{
    bool compara_con_dtb(void* dtb) {
        return dtb_coincide_pid(dtb, pid, flag);
    }
    return list_remove_by_condition(lista, compara_con_dtb);
}
// va sin parentesis compara_con_dtb porque tiene que llamar al puntero a la funcion despues de que hacer list_find

void liberar_cpu(int socket)
{
    t_cpu* cpu_actual = cpu_con_socket(lista_cpu, socket);
    cpu_actual->estado = CPU_LIBRE;
}

bool coincide_socket(int socket, void* cpu) {
	return ((t_cpu*)cpu)->socket == socket;
}

t_cpu* cpu_con_socket(t_list* lista, int socket) {
    bool compara_cpu(void* cpu) {
        return coincide_socket(socket, cpu);
    }
    return list_find(lista, compara_cpu);
}

bool esta_libre_cpu (void* cpu) {
	t_cpu* cpu_recv = (t_cpu*) cpu;
    return (cpu_recv->estado == CPU_LIBRE);
}

bool hay_cpu_libre() {
    return list_any_satisfy(lista_cpu, esta_libre_cpu);
}

//Funciones booleanas

bool permite_multiprogramacion() {
    return procesos_en_memoria < MULTIPROGRAMACION;
}

bool esta_en_memoria(DTB_info *info_dtb)
{
    return (info_dtb->estado == DTB_LISTO || info_dtb->estado == DTB_EJECUTANDO || info_dtb->estado == DTB_BLOQUEADO);
}

bool dummy_creado(DTB *dtb)
{
    return (dtb_encuentra(lista_listos, dtb->gdtPID, DUMMY) != NULL) || (dtb_encuentra(lista_ejecutando, dtb->gdtPID, DUMMY) != NULL);
}

//

DTB *dtb_buscar_en_todos_lados(u_int32_t pid, DTB_info **info_dtb, t_list **lista_actual)
{
    DTB *dtb_encontrado;
    for(u_int32_t i = 0; i < (list_size(lista_estados)); i++)
    {   
        *lista_actual = list_get(lista_estados, i);
        dtb_encontrado = dtb_encuentra(*lista_actual, pid, 1);
        if(dtb_encontrado != NULL)
        {
            *info_dtb = info_asociada_a_pid(dtb_encontrado->gdtPID); //Devuelve el DTB que se guarda localmente en planificador
            break;
        }
    }
    return dtb_encontrado;
}

void dtb_imprimir_basico(void *_dtb)
{
    DTB *dtb = (DTB *)_dtb;
    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

    switch(dtb->flagInicializacion)
    {
        case DUMMY:
        {
            printf("DTB Dummy asociado a GDT %d\n", dtb->gdtPID);
            break;
        }
        case GDT:
        {
            ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
            printf( "PID: %i\n"
                    "Escriptorio: %s%s\n",
                    dtb->gdtPID, escriptorio->path,
                    (escriptorio->cantLineas) ? (", cantidad de lineas: %d", escriptorio->cantLineas) : "");
            break;
        }
    }
}

void dtb_imprimir_polenta(void *_dtb)
// Agregar lista de recursos.
{
    DTB *dtb = (DTB *)_dtb;
    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
    dtb_imprimir_basico(dtb);
    printf( "Estado: %s\n"
            "Program Counter: %i\n"
            "Ultima CPU: %i\n"
            "Tiempo de respuesta: %f\n"
            "Proceso finalizado por usuario: %s\n"
            "Archivos Abiertos: %i\n",
            Estados[info_dtb->estado], dtb->PC, info_dtb->socket_cpu,
            info_dtb->tiempo_respuesta,
            (info_dtb->kill) ? "si" : "no",
            list_size(dtb->archivosAbiertos) - 1);
}

void mostrar_proceso(void *_dtb)
{ 
	DTB* dtb = (DTB *)_dtb;
    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
    switch(info_dtb->estado)
    {
        case DTB_NUEVO:
        {
            dtb_imprimir_basico(dtb);
            printf("Estado: %s\n", Estados[info_dtb->estado]);
            break;
        }
        default:
        {
            dtb_imprimir_polenta(dtb);
            break;
        }
    }

    for(int i = 1; i < list_size(dtb->archivosAbiertos); i++)
    {
        ArchivoAbierto *archivo = list_get(dtb->archivosAbiertos, i);
        mostrar_archivo(archivo, i);
    }
}

void mostrar_archivo(void *_archivo, int index) // queda en void *_archivo por si volvemos a list_iterate
{
    ArchivoAbierto *archivo = (ArchivoAbierto *) _archivo;
    printf( "Archivo %d:\n"
            "Directorio: %s, cantidad de lineas:%s\n",
            // Agregar si se agregan campos a ArchivoAbierto
            index,
            archivo->path, (archivo->cantLineas) ? (", cantidad de lineas: %i", archivo->cantLineas) : "No cargadas todavia");
}

//Funciones de Consola
void ejecutar(char* path) {
	DTB* dtb_nuevo = dtb_crear(0, path, GDT);
}

void status()
{
    for(int i = 0; i < (list_size(lista_estados)); i++)
    {
        t_list *lista_mostrar = list_get(lista_estados, i);
        printf("Cantidad de procesos en Estado %s: %i\n", Estados[i], list_size(lista_mostrar));
        // if(list_size(lista_mostrar) != 0)printf("Informacion asociada a los procesos de la lista:\n");
	    if (list_size(lista_mostrar) != 0)
            list_iterate(lista_mostrar, dtb_imprimir_basico);

    }
    return;
}

void gdt_status(u_int32_t pid)
{
    t_list *lista_actual;
    DTB_info *info_dtb;
    DTB *dtb_status = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);

    if (dtb_status != NULL) mostrar_proceso(dtb_status);
    else printf("El proceso %i no esta en el sistema\n", pid);
}

void finalizar(u_int32_t pid)
{
    t_list *lista_actual;
    DTB_info *info_dtb;

    if(pid <= numero_pid)
    {
    DTB *dtb_finalizar = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);
    /* lista_actual queda modificada con la lista donde está el pid que busco */
    if (dtb_finalizar != NULL)
        manejar_finalizar(dtb_finalizar, pid, info_dtb, lista_actual);
    else
        printf("El proceso con PID %i ha sido finalizado, puede revisar Archivo Finalizados para mas informacion", pid);
    }
    else
        printf("\nNo se tienen registros del proceso con PID %i en el sistema.\n", pid);
}

void manejar_finalizar(DTB *dtb, u_int32_t pid, DTB_info *info_dtb, t_list *lista_actual)
{
    info_dtb->kill = true;
    switch(info_dtb->estado)
    {
        case DTB_NUEVO:
        {
            printf("El GDT con PID %d esta en nuevos\n", pid);
            if(dtb_encuentra(lista_listos, pid, DUMMY))
            {
                bloquear_dummy(lista_listos, pid);
                loggear_finalizacion(dtb, info_dtb);
                dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
            }
            break;
        }
        case DTB_LISTO:
        {
        	printf("El GDT con PID %d esta listo\n", pid);
        	loggear_finalizacion(dtb, info_dtb);
            list_iterate(info_dtb->recursos, forzar_signal);
            dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
            enviar_finalizar_dam(pid);
            break;
        }
        case DTB_EJECUTANDO:
        {
        	printf("El GDT con PID %d esta ejecutando\n", pid);
            enviar_finalizar_cpu(pid, info_dtb->socket_cpu);
            break;
        }
        case DTB_BLOQUEADO:
        {
        	printf("El GDT con PID %d esta bloqueado\n", pid);
        	loggear_finalizacion(dtb, info_dtb);
            list_iterate(info_dtb->recursos, forzar_signal);
            enviar_finalizar_dam(pid);
            break;
        }
        case DTB_FINALIZADO:
        {
            printf("El GDT con PID %d ya fue finalizado\n", pid);
            break;
        }
        default:
        {
            printf("El proceso con PID %d no esta en ninguna cola\n", pid);
            break;
        }
    }
    printf("El proceso de PID %d se ha movido a la cola de Finalizados\n", dtb->gdtPID);
}


void loggear_finalizacion(DTB* dtb, DTB_info* info_dtb)
{
	log_info(logger_fin, "El proceso con PID %d fue finalizado."
"\n											 Flag: %i"
"\n											 Cantidad de archivos abiertos: %d"
"\n											 Tiempo de respuesta final: %f milisegundos"
"\n											 Proceso Finalizado por Usuario: %s"
"\n											 Ultimo Estado: %s",
				              dtb->gdtPID, dtb->flagInicializacion,
							  list_size(dtb->archivosAbiertos), info_dtb->tiempo_respuesta,
							  (info_dtb->kill)? "Si" : "No",
							  (info_dtb->kill) ? "DTB_Ejecutando" : Estados[info_dtb->estado]);
}


void pasaje_a_ready(u_int32_t *pid)
{
    bloquear_dummy(lista_ejecutando, *pid);
    notificar_al_plp(*pid);
    log_info(logger, "%d pasa a lista de procesos listos", pid);
}

void enviar_finalizar_dam(u_int32_t pid)
{
    Paquete *paquete = malloc(sizeof(Paquete));
    paquete->Payload = malloc(sizeof(u_int32_t));
    memcpy(paquete->Payload, &pid, sizeof(u_int32_t));
    paquete->header = cargar_header(sizeof(u_int32_t), FIN_BLOQUEADO, SAFA);
    EnviarPaquete(socket_diego, paquete);
    free(paquete);
    printf("Le mande a dam que finalice GDT %d\n", pid);
}

void enviar_finalizar_cpu(u_int32_t pid, int socket_cpu)
{
    Paquete *paquete = malloc(sizeof(Paquete));
    paquete->Payload = malloc(sizeof(u_int32_t));
    memcpy(paquete->Payload, &pid, sizeof(u_int32_t));
    paquete->header = cargar_header(sizeof(u_int32_t), FINALIZAR, SAFA);
    EnviarPaquete(socket_cpu, paquete);
    free(paquete);
    printf("Le mande a cpu que finalice GDT %d\n", pid);
}

void forzar_signal(void *_recurso)
{
    t_recurso *recurso = (t_recurso *)_recurso;
    dtb_signal(recurso);
}

void dtb_signal(t_recurso *recurso)
{
	u_int32_t *pid = list_remove(recurso->pid_bloqueados, 0);

	if (pid != NULL)
	{
	DTB *dtb = dtb_encuentra(lista_bloqueados, *pid, GDT);
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC , DTB_LISTO, info_dtb->socket_cpu);
	recurso_asignar_a_pid(recurso, dtb->gdtPID);
	}
}


void recurso_asignar_a_pid(t_recurso *recurso, u_int32_t pid)
{
	DTB_info *info_dtb = info_asociada_a_pid(pid);
	list_add(info_dtb->recursos, recurso);
}

clock_t* t_ini;
clock_t* t_fin;
float secs;
double contador_procesos_bloqueados = 0, t_total = 0;
float t_rta;

float medir_tiempo(int signal, clock_t* tin_rcv, clock_t* tfin_rcv){

	switch (signal){

	    case 1:
	    {
	     t_ini = tin_rcv;
	     *t_ini = clock();
	     break; //tiempo al inicio
	    }

	    case 0:
	    {
	     t_ini = tin_rcv;
	     t_fin = tfin_rcv;
	     *t_fin = clock();//tiempo al final
	    secs = ((double) ((*t_fin) - (*t_ini)) / CLOCKS_PER_SEC)*1000;
	    contador_procesos_bloqueados++;
	    t_total+=secs;
	    break;
	    }
	    default:
	    {

	    }
	  }
	  return secs;
	  t_rta = t_total/contador_procesos_bloqueados;
}

void metricas(){

	printf("\nEl Tiempo de Respuesta del sistema es: %f milisegundos", t_rta);
}


// FUNCIONES VIEJAS QUE PROBABLEMENTE NO SE USEN. LAS DEJO POR LAS DUDAS

void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2) {
    DTB* DTB_a_mover = list_remove(lista1, 0);
    list_add(lista2, DTB_a_mover);
}

// DTB *dtb_reemplazar_de_lista(DTB *dtb, t_list *source, t_list *dest, Estado estado)
// {
//     DTB *dtb_viejo = dtb_remueve(source, dtb->gdtPID);
//     DTB_info *info_vieja = info_asociada_a_pid(dtb_viejo->gdtPID);
//     DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
//     info_dtb->tiempo_respuesta = info_vieja->tiempo_respuesta;
//     info_dtb->tiempo_ini = info_vieja->tiempo_ini;
//     info_dtb->tiempo_fin = info_vieja->tiempo_fin;
//     info_dtb->kill = info_vieja->kill;
//     info_dtb->estado = estado;
//     dtb_liberar(dtb_viejo);
//     list_add(dest, dtb);

//     return dtb;
// }

void dtb_finalizar_desde(DTB *dtb, t_list *source)
{
    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

    info_dtb->estado = DTB_FINALIZADO;
    mover_dtb_de_lista(dtb, source, lista_finalizados);
    dtb_liberar(dtb);
    procesos_finalizados++;
}

DTB *mover_dtb_de_lista(DTB *dtb, t_list *source, t_list *dest)
{
    list_add(dest, dtb);
    return dtb_remueve(source, dtb->gdtPID, dtb->flagInicializacion);
}

void dummy_finalizar(DTB *dtb) {
    DTB *dummy = dtb_remueve(lista_ejecutando, dtb->gdtPID, DUMMY);
    dtb_liberar(dummy);
}