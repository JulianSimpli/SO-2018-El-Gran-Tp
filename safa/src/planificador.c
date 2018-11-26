#include "planificador.h"

u_int32_t numero_pid = 0, 
procesos_finalizados = 0,
sentencias_globales_del_diego = 0, sentencias_totales = 0;

char* Estados[5] = {"Nuevo", "Listo", "Ejecutando", "Bloqueado", "Finalizado"};

//Hilo planificador largo plazo
void planificador_largo_plazo()
{
    while(true)
    {
        sem_wait(&sem_ejecutar);
        sem_wait(&sem_multiprogramacion);
        crear_dummy();
    }
}

void crear_dummy()
{
    for(int i = 0; i < list_size(lista_nuevos); i++)
    {
        DTB *dtb_cargar = list_get(lista_nuevos, i);
        if(!dummy_creado(dtb_cargar))
        {   
            desbloquear_dummy(dtb_cargar);
            break;
        }
    }
}

void pasaje_a_ready(u_int32_t pid)
{
    bloquear_dummy(lista_ejecutando, pid);
    notificar_al_plp(pid);
    log_info(logger, "%d pasa a lista de procesos listos", pid);
}

void notificar_al_plp(u_int32_t pid)
{
    DTB* DTB_a_mover = dtb_remueve(lista_nuevos, pid, 1);
    list_add(lista_listos, DTB_a_mover);
}

bool dummy_creado(DTB *dtb)
{
    return (dtb_encuentra(lista_listos, dtb->gdtPID, DUMMY) != NULL) || (dtb_encuentra(lista_ejecutando, dtb->gdtPID, DUMMY) != NULL);
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
    if(!list_is_empty(lista_prioridad))
        ejecutar_primer_dtb_prioridad();
    else
        ejecutar_primer_dtb_listo();
}
void planificar_iobound()
{
    bool io_bound_first(void *_dtb_mayor, void *_dtb_menor)
    {
        return ((DTB *)_dtb_mayor)->entrada_salidas > ((DTB *)_dtb_menor)->entrada_salidas;
    }
    list_sort(lista_listos, io_bound_first);
    ejecutar_primer_dtb_listo();
}

//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo() {
    t_cpu *cpu_libre = cpu_buscar_libre();
    DTB *dtb_exec = list_get(lista_listos, 0);
    
    Paquete *paquete = malloc(sizeof(Paquete));
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
    dtb_actualizar(dtb_exec, lista_listos, lista_ejecutando, dtb_exec->PC, DTB_EJECUTANDO, cpu_libre->socket);
    free(paquete->Payload);
    free(paquete);
}

void ejecutar_primer_dtb_prioridad()
{
    t_cpu *cpu_libre = cpu_buscar_libre();
    DTB *dtb = list_get(lista_prioridad, 0);
    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

    Paquete *paquete = malloc(sizeof(Paquete));
    int tamanio_dtb = 0;
    paquete->Payload = DTB_serializar(dtb, &tamanio_dtb);
    paquete->Payload = realloc(paquete->Payload, tamanio_dtb + sizeof(u_int32_t));
    memcpy(paquete->Payload + tamanio_dtb, &info_dtb->quantum_faltante, sizeof(u_int32_t));
    paquete->header = cargar_header(tamanio_dtb + sizeof(u_int32_t), ESDTBQUANTUM, SAFA);
    EnviarPaquete(cpu_libre->socket, paquete);
    free(paquete->Payload);
    free(paquete);

    dtb_actualizar(dtb, lista_prioridad, lista_ejecutando, dtb->PC, DTB_EJECUTANDO, cpu_libre->socket);
}

t_cpu *cpu_buscar_libre()
{
    t_cpu* cpu_libre = list_find(lista_cpu, esta_libre_cpu);
    cpu_libre->estado = CPU_OCUPADA;

    return cpu_libre;
}

//Funciones asociadas a DTB
DTB *dtb_crear(u_int32_t pid, char* path, int flag_inicializacion)
{
	DTB *dtb_nuevo = malloc(sizeof(DTB));
    dtb_nuevo->flagInicializacion = flag_inicializacion;
    dtb_nuevo->PC = 1;
    dtb_nuevo->entrada_salidas = 0;
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
    info_dtb->recursos_asignados = list_create();
    info_dtb->quantum_faltante = 0;
    info_dtb->sentencias_en_nuevo = 0;
    info_dtb->sentencias_al_diego = 0;
    info_dtb->sentencias_hasta_finalizar = 0;
    //*info_dtb->tiempo_ini
    //*info_dtb->tiempo_fin
    list_add(lista_info_dtb, info_dtb);

    return info_dtb;
}

DTB *dtb_actualizar(DTB *dtb, t_list *source, t_list *dest, u_int32_t pc, Estado estado, int socket)
{
    if(dtb_encuentra(source, dtb->gdtPID, dtb->flagInicializacion) != NULL)
        dtb_remueve(source, dtb->gdtPID, dtb->flagInicializacion);
    else
        printf("No se encontro al DTB en la lista de origen.\n");

    list_add(dest, dtb);
    dtb->PC = pc;

    DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
    if(info_dtb != NULL)
    {
        info_dtb_actualizar(estado, socket, info_dtb);
        switch(estado)
        {
            case DTB_FINALIZADO:
            {
                procesos_finalizados++;
                advertencia();
                break;
            }
            default:
                break;
        }
    }

    return dtb;
}

DTB_info* info_dtb_actualizar(Estado estado, int socket, DTB_info *info_dtb)
{
	info_dtb->socket_cpu = socket;
	info_dtb->estado = estado;
    return info_dtb;
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

//Funciones de Consola
void ejecutar(char* path)
{
	DTB* dtb_nuevo = dtb_crear(0, path, GDT);
    sem_post(&sem_ejecutar);
}

// Status
void status()
{
    for(int i = 0; i < (list_size(lista_estados)); i++)
    {
        t_list *lista_mostrar = list_get(lista_estados, i);
        printf("Cola de Estado %s:\n", Estados[i]);
	    if (list_size(lista_mostrar) != 0)
        {
            contar_dummys_y_gdt(lista_mostrar);
            list_iterate(lista_mostrar, dtb_imprimir_basico);
        }
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

void contar_dummys_y_gdt(t_list* lista)
{
    bool es_dummy(void* dtb)
    {
        return (((DTB*)dtb)->flagInicializacion== DUMMY);
    }
        int cant_dummys = list_count_satisfying(lista, es_dummy);
        int cant_gdt = list_size(lista) - cant_dummys;
    if(cant_dummys != 0)
        printf("Cantidad de DUMMYS: %i\n", cant_dummys);
    printf("Cantidad de GDTs: %i\n", cant_gdt);
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
                    "Escriptorio: %s",
                    dtb->gdtPID, escriptorio->path);
            if (escriptorio->cantLineas)
                printf(", cantidad de lineas: %d", escriptorio->cantLineas);
            printf("\n");
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

    printf("Recursos retenidos por el proceso: %d\n", list_size(info_dtb->recursos_asignados));
    for(int j = 0; j < list_size(info_dtb->recursos_asignados); j++)
    {
        t_recurso_asignado *recurso_asignado = list_get(info_dtb->recursos_asignados, j);
        printf( "Recurso %d: %s\n",
                j, recurso_asignado->recurso->id);
    }
}

void mostrar_archivo(void *_archivo, int index) // queda en void *_archivo por si volvemos a list_iterate
{
    ArchivoAbierto *archivo = (ArchivoAbierto *) _archivo;
    printf( "Archivo %d:\n"
            "Directorio: %s",
            // Agregar si se agregan campos a ArchivoAbierto
            index, archivo->path);
    if (archivo->cantLineas) 
        printf(", cantidad de lineas: %i", archivo->cantLineas);
    else 
        printf(", No cargadas todavia");
    printf("\n");
}

// Finalizar
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
            if(!dummy_creado(dtb))
            {
                loggear_finalizacion(dtb, info_dtb);
                dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);                
            }
            else if(dtb_encuentra(lista_listos, pid, DUMMY))
            {
                bloquear_dummy(lista_listos, pid);
                loggear_finalizacion(dtb, info_dtb);
                dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
                sem_post(&sem_multiprogramacion);
            }
            break;
        }
        case DTB_LISTO:
        {
        	printf("El GDT con PID %d esta listo\n", pid);
        	loggear_finalizacion(dtb, info_dtb);
            limpiar_recursos(info_dtb);
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
            manejar_finalizar_bloqueado(dtb, pid, info_dtb, lista_actual);
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

void manejar_finalizar_bloqueado(DTB* dtb, u_int32_t pid, DTB_info *info_dtb, t_list *lista_actual)
{
    t_recurso *recurso = recurso_bloqueando_pid(pid);
    if(recurso != NULL)
    {
        printf("El GDT %d esta bloqueado esperando por el recurso %s", pid, recurso->id);
        printf("Se finalizara liberando al recurso, junto a todos los que tiene retenidos");
        bool coincide_pid(void *_pid_recurso)
        {
            int *pid_recurso = (int *)_pid_recurso;
            return *pid_recurso == pid;
        }
        list_remove_by_condition(recurso->pid_bloqueados, coincide_pid);
        loggear_finalizacion(dtb, info_dtb);
        limpiar_recursos(info_dtb);
        dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
        enviar_finalizar_dam(pid);
    }
    else
        printf( "El GDT %d esta bloqueado esperando que se realice una operacion\n"
                "Se esperara a que se desbloquee al proceso para finalizarlo\n", pid);
}

t_recurso *recurso_bloqueando_pid(u_int32_t pid)
{
    bool tiene_el_pid(void *_recurso)
    {
        bool coincide_pid(void *_pid_recurso)
        {
            int *pid_recurso = (int *)_pid_recurso;
            return *pid_recurso == pid;
        }
        t_recurso *recurso = (t_recurso *)_recurso;
        return list_any_satisfy(recurso->pid_bloqueados, coincide_pid);
    }
    
    return list_find(lista_recursos_global, tiene_el_pid);
}

void loggear_finalizacion(DTB* dtb, DTB_info* info_dtb)
{
	log_info(logger_fin, "El proceso con PID %d fue finalizado."
"\n											 Tiempo de respuesta final: %f milisegundos"
"\n											 Proceso Finalizado por Usuario: %s"
"\n											 Ultimo Estado: %s"
"\n                                          Cantidad de archivos abiertos: %d"
"\n                                          Recursos retenidos por el proceso: %d",
    dtb->gdtPID, dtb->flagInicializacion,
	info_dtb->tiempo_respuesta,
	(info_dtb->kill)? "Si" : "No",
	Estados[info_dtb->estado],
	list_size(dtb->archivosAbiertos));

	FILE* logger_fin = fopen("/home/utnso/Escritorio/KE/Archivos backup TP/TPSO/tp-2018-2c-Nene-Malloc/safa/src/Archivos Finalizados.log", "a");
		for(int i=0; i<list_size(info_dtb->recursos_asignados); i++){
			t_recurso* recurso;
			recurso = list_get(info_dtb->recursos_asignados, i);
			fprintf(logger_fin, "\nRecurso retenido n° %i: %s: ",recurso->id);
		}
	fclose(logger_fin);
}

void enviar_finalizar_dam(u_int32_t pid)
{
    Paquete *paquete = malloc(sizeof(Paquete));
    paquete->Payload = malloc(sizeof(u_int32_t));
    memcpy(paquete->Payload, &pid, sizeof(u_int32_t));
    paquete->header = cargar_header(sizeof(u_int32_t), FIN_BLOQUEADO, SAFA);
    EnviarPaquete(socket_diego, paquete);
    free(paquete->Payload);
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
    free(paquete->Payload);
    free(paquete);
    printf("Le mande a cpu que finalice GDT %d\n", pid);
}

// Recursos
t_recurso *recurso_crear(char *id_recurso, int valor_inicial)
{
	t_recurso *recurso = malloc(sizeof(t_recurso));
	recurso->id = malloc(strlen(id_recurso) + 1);
	strcpy(recurso->id, id_recurso);
	recurso->semaforo = valor_inicial;
	recurso->pid_bloqueados = list_create();

	list_add(lista_recursos_global, recurso);

	return recurso;
}

void recurso_liberar(void *_recurso)
{
	t_recurso *recurso = (t_recurso *)_recurso;
	free(recurso->id);
	list_destroy(recurso->pid_bloqueados);
	free(recurso);
}

bool recurso_coincide_id(void *recurso, char *id)
{
	return !strcmp(((t_recurso *)recurso)->id, id);
}

t_recurso *recurso_encontrar(char *id_recurso)
{
	bool comparar_id(void *recurso)
	{
		return recurso_coincide_id(recurso, id_recurso);
	}

	return list_find(lista_recursos_global, comparar_id);
}

void limpiar_recursos(DTB_info *info_dtb)
{
    for(int i = 0; i < list_size(info_dtb->recursos_asignados); i++)
    {
        t_recurso_asignado *recurso_asignado = list_get(info_dtb->recursos_asignados, i);
        forzar_signal(recurso_asignado);
    }
    list_clean_and_destroy_elements(info_dtb->recursos_asignados, free);
}

void forzar_signal(t_recurso_asignado *recurso_asignado)
{
    while(recurso_asignado->instancias--)
        dtb_signal(recurso_asignado->recurso);
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
    else
        recurso->semaforo++;
}

void recurso_asignar_a_pid(t_recurso *recurso, u_int32_t pid)
{
	DTB_info *info_dtb = info_asociada_a_pid(pid);
    t_recurso_asignado *rec_asignado = malloc(sizeof(t_recurso_asignado));

    bool esta_asignado (void * _recurso_asignado)
    {
        t_recurso_asignado *recurso_asignado = (t_recurso_asignado *)_recurso_asignado;
        return recurso_coincide_id(recurso_asignado->recurso, recurso->id);
    }
    rec_asignado = list_find(info_dtb->recursos_asignados, esta_asignado);

    if(rec_asignado == NULL)
    {
        rec_asignado->recurso = recurso;
        rec_asignado->instancias = 1;
        list_add(info_dtb->recursos_asignados, rec_asignado);
    }
    else
        rec_asignado->instancias++;

}
//
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

void gdt_metricas(u_int32_t pid)
{
    metricas();
    DTB_info *info_dtb = info_asociada_a_pid(pid);
    printf("Estadisticas del GDT %d\n", pid);
    printf("Espero en nuevo %d sentencias\n", info_dtb->sentencias_en_nuevo);
    printf("Uso a  \"El diego\" %d veces", info_dtb->sentencias_al_diego);
    printf("El ultimo tiempo de respuesta de %f", info_dtb->tiempo_respuesta);
}

void metricas()
{
    printf("Estadisticas de la fecha\n");
    if(numero_pid)
    {
        float sentencias_promedio_diego = calcular_sentencias_promedio_diego();
        printf("Sentencias ejecutadas promedio del sistema que usaron a \"El Diego\": %f"
        ,sentencias_promedio_diego);
    }
    else printf("Todavia no empezo el campeonato\n");
    if(procesos_finalizados)
    {
        float sentencias_promedio_hasta_finalizar = calcular_sentencias_promedio_hasta_finalizar();
        printf("Sentencias ejecutadas promedio del sistema para que un dtb termine en la cola de EXIT: %f"
        , sentencias_promedio_hasta_finalizar);
    }
    else printf("Ningun GDT finalizo hasta el momento\n");
    if(sentencias_totales)
    {
        float porcentaje_al_diego = sentencias_globales_del_diego / sentencias_totales;
        printf("Porcentaje de las sentencias ejecutadas promedio que fueron a \"El Diego\": %f"
        , porcentaje_al_diego);
    }
    else printf("Ninguna sentencia fue ejecutada hasta el momento");
    printf("Tiempo de respuesta promedio del sistema: %f milisegundos\n", t_rta);
}

void* list_fold(t_list* self, void* seed, void*(*operation)(void*, void*))
{
	t_link_element* element = self->head;
	void* result = seed;

	while(element != NULL)
	{
		result = operation(result, element->data);
		element = element->next;
	}

	return result;
}

float calcular_sentencias_promedio_diego()
{
    int _sumar_sentencias_al_diego(void *_acum, void * _info_dtb)
    {
        int acum = (int)_acum;
        DTB_info *info_dtb = (DTB_info *)_info_dtb;
        return acum + info_dtb->sentencias_al_diego;
    }
    int sentencias_totales = list_fold(lista_info_dtb, 0, (void *)_sumar_sentencias_al_diego);
    
    return sentencias_totales/numero_pid;
}

float calcular_sentencias_promedio_hasta_finalizar()
{
    int _sumar_sentencias_hasta_finalizar(void *_acum, void *_info_dtb)
    {
        int acum = (int)_acum;
        DTB_info *info_dtb = (DTB_info *)_info_dtb;
        return acum + info_dtb->sentencias_hasta_finalizar;
    }
    t_list *finalizados = list_filter(lista_info_dtb, ya_finalizo);
    if(finalizados == NULL)
        return 0;
    int sentencias_hasta_finalizar = list_fold(finalizados, 0, (void*)_sumar_sentencias_hasta_finalizar);

    return sentencias_hasta_finalizar/procesos_finalizados;
}

bool ya_finalizo(void *_info_dtb)
{
    DTB_info *info_dtb = (DTB_info *)_info_dtb;
    return info_dtb->estado == DTB_FINALIZADO;
}

/*
1. Cant. de sentencias ejecutadas que esperó un DTB en la cola NEW
2. Cant.de sentencias ejecutadas prom. del sistema que usaron a “El Diego”
3. Cant. de sentencias ejecutadas prom. del sistema para que un DTB termine en la cola EXIT
4. Porcentaje de las sentencias ejecutadas promedio que fueron a “El Diego”
5. Tiempo de Respuesta promedio del Sistema
 */

// Liberar Memoria

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
    list_remove_and_destroy_by_condition(lista_info_dtb, coincide_info, info_liberar_completo);
}

void info_liberar_completo(void* info_dtb)
{
    list_clean_and_destroy_elements(((DTB_info*)info_dtb)->recursos_asignados, free);
    free(((DTB_info*)info_dtb)->tiempo_fin);
    free(((DTB_info*)info_dtb)->tiempo_ini);
    free(info_dtb);
}

void advertencia()
{
    int c, procesos_liberados, procesos_eliminar;
    if(procesos_finalizados > 60)
    {
        printf("\nHay mas de 60 procesos finalizados, seleccione una opcion:\n"
        "1: Eliminar todos los procesos.\n"
        "2: Elegir la cantidad de procesos a eliminar\n");
        scanf("%i",&c);
        switch(c)
        {
        case 1:
        {
            procesos_liberados = list_size(lista_finalizados);
            liberar_memoria();
            printf("Se liberaron %i procesos\n", procesos_liberados);
            log_info(logger, "Se finalizaron %i procesos por consola", procesos_liberados);
            break;
        }
        case 2:
        {
            printf("\nIngrese la cantidad de procesos a eliminar: ");
            scanf("%i", &procesos_eliminar);
            liberar_parte_de_memoria(procesos_eliminar);
            log_info(logger, "Se finalizaron los primeros %i procesos por consola", procesos_eliminar);
            printf("Se liberaron los primeros %i procesos\n", procesos_eliminar);
            break;
        }
        }
    }
}
void liberar_memoria()
{
    list_clean_and_destroy_elements(lista_finalizados, dtb_liberar);
    procesos_finalizados -= list_size(lista_finalizados);
}
void liberar_parte_de_memoria(int procesos_a_eliminar)
{
    for(int i = 0; i < procesos_a_eliminar; i++)
        list_remove_and_destroy_element(lista_finalizados, i, dtb_liberar);
    procesos_finalizados -= procesos_a_eliminar;
}


// FUNCIONES VIEJAS QUE NO SE USAN. LAS DEJO POR LAS DUDAS

// void mover_primero_de_lista1_a_lista2(t_list* lista1, t_list* lista2) {
//     DTB* DTB_a_mover = list_remove(lista1, 0);
//     list_add(lista2, DTB_a_mover);
// }

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

// void dtb_finalizar_desde(DTB *dtb, t_list *source)
// {
//     DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

//     info_dtb->estado = DTB_FINALIZADO;
//     mover_dtb_de_lista(dtb, source, lista_finalizados);
//     dtb_liberar(dtb);
//     procesos_finalizados++;
// }

// DTB *mover_dtb_de_lista(DTB *dtb, t_list *source, t_list *dest)
// {
//     list_add(dest, dtb);
//     return dtb_remueve(source, dtb->gdtPID, dtb->flagInicializacion);
// }

// void dummy_finalizar(DTB *dtb) {
//     DTB *dummy = dtb_remueve(lista_ejecutando, dtb->gdtPID, DUMMY);
//     dtb_liberar(dummy);
// }

// bool permite_multiprogramacion() {
//     return procesos_en_memoria < MULTIPROGRAMACION;
// }

// bool esta_en_memoria(DTB_info *info_dtb)
// {
//     return (info_dtb->estado == DTB_LISTO || info_dtb->estado == DTB_EJECUTANDO || info_dtb->estado == DTB_BLOQUEADO);
// }
