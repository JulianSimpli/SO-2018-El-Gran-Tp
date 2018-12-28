#include "planificador.h"
#include <commons/collections/list.h>

u_int32_t numero_pid = 0,
procesos_actuales = 0;

double procesos_finalizados = 0,
sentencias_globales_del_diego = 0,
sentencias_totales = 0,
trt = 0, // total response time
average_rt = 0; // average response time

int procesos_a_esperar = 0;

char *Estados[5] = {"Nuevo", "Listo", "Ejecutando", "Bloqueado", "Finalizado"};

//Hilo planificador largo plazo
void planificador_largo_plazo()
{
	while (true)
	{
		sem_wait(&sem_ejecutar);
		sem_wait(&sem_multiprogramacion);
		log_debug(logger, "El grado de multiprogramacion es suficiente");
		crear_dummy();
	}
}

void crear_dummy()
{
	log_info(logger, "Empiezo a buscar dummy para crear");
	for (int i = 0; i < list_size(lista_nuevos); i++)
	{
		DTB *dtb_cargar = list_get(lista_nuevos, i);
		log_info(logger, "Voy a intentar crear el dummy del GDT %d", dtb_cargar->gdtPID);
		if (!dummy_creado(dtb_cargar))
		{
			log_info(logger, "Voy a crear el dummy del GDT %d", dtb_cargar->gdtPID);
			desbloquear_dummy(dtb_cargar);
			break;
		}
	}
}

void notificar_al_plp(u_int32_t pid)
{
	DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
	dtb_actualizar(dtb, lista_nuevos, lista_listos, dtb->PC, DTB_LISTO, 0);
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	info_dtb->tiempo_ini = medir_tiempo();
}

bool dummy_creado(DTB *dtb)
{
	return (dtb_encuentra(lista_listos, dtb->gdtPID, DUMMY) != NULL) || (dtb_encuentra(lista_ejecutando, dtb->gdtPID, DUMMY) != NULL) || (dtb_encuentra(lista_bloqueados, dtb->gdtPID, DUMMY) != NULL);
}

void bloquear_dummy(t_list *lista, u_int32_t pid)
{
	bool compara_dummy(void *dtb)
	{
		return dtb_coincide_pid(dtb, pid, DUMMY);
	}
	log_debug(logger, "Tamanio COLA BLOQUEADOS PRE-DESTRUIR DUMMY %d", list_size(lista_bloqueados));
	list_remove_and_destroy_by_condition(lista, compara_dummy, dtb_liberar);
	log_debug(logger, "Tamanio COLA BLOQUEADOS POST-DESTRUIR DUMMY %d", list_size(lista_bloqueados));
	log_info(logger, "Dummy del GDT %d destruido", pid);
}

void desbloquear_dummy(DTB *dtb_nuevo)
{
	ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
	log_debug(logger, "El escriptorio a ejecutar es %s\n", escriptorio->path);
	dtb_crear(dtb_nuevo->gdtPID, escriptorio->path, DUMMY); //Tambien lo agrega a la lista
}

//Hilo planificador corto plazo
void planificador_corto_plazo()
{
	while (true)
	{
		sem_wait(&sem_listos);
		ordenar_cpu_por_menor_ejecutados();
		usleep(RETARDO_PLANIF*1000);

		if (!strcmp(ALGORITMO_PLANIFICACION, "FIFO"))
			planificar_fifo();
		else if (!strcmp(ALGORITMO_PLANIFICACION, "RR"))
			planificar_rr();
		else if (!strcmp(ALGORITMO_PLANIFICACION, "VRR"))
			planificar_vrr();
		else if (!strcmp(ALGORITMO_PLANIFICACION, "IOBF"))
			planificar_iobound();
		else
		{
			log_debug(logger, "No se conoce el algoritmo. Cambielo desde SAFA.config");
			usleep(10*1000);
		}
	}
}

void planificar_fifo()
{
	ejecutar_primer_dtb_listo();
}
void planificar_rr()
{
	ejecutar_primer_dtb_listo();
}
void planificar_vrr()
{
	ejecutar_primer_dtb_prioridad();
	ejecutar_primer_dtb_listo();
}

void ordenar_cpu_por_menor_ejecutados()
{
	bool mas_ejecuto(void *_cpu_mayor, void *_cpu_menor)
	{
		return ((t_cpu *)_cpu_mayor)->dtb_ejecutados < ((t_cpu *)_cpu_menor)->dtb_ejecutados;
	}
	list_sort(lista_cpu, mas_ejecuto);
}

void planificar_iobound()
{
	bool io_bound_first(void *_dtb_mayor, void *_dtb_menor)
	{
		return ((DTB *)_dtb_mayor)->entrada_salidas > ((DTB *)_dtb_menor)->entrada_salidas;
	}

	list_sort(lista_prioridad, io_bound_first);	
	list_sort(lista_listos, io_bound_first);

	usleep(RETARDO_PLANIF*1000);
	ejecutar_primer_dtb_prioridad();
	ejecutar_primer_dtb_listo();
}

//Funciones planificador corto plazo
void ejecutar_primer_dtb_listo()
{
	if(list_is_empty(lista_listos) || !hay_cpu_libre())
		return;
	
	t_cpu *cpu_libre = list_find(lista_cpu, esta_libre_cpu);
	cpu_libre->estado = CPU_OCUPADA;
	log_debug(logger, "Esta libre la cpu %d\n", cpu_libre->socket);
	DTB *dtb_exec = list_get(lista_listos, 0);

	Paquete *paquete = malloc(sizeof(Paquete));
	int tamanio_DTB = 0;
	paquete->Payload = DTB_serializar(dtb_exec, &tamanio_DTB);

	sem_wait(&sem_dummy);
	switch (dtb_exec->flagInicializacion)
	{
	case DUMMY:
	{
		paquete->header = cargar_header(tamanio_DTB, ESDTBDUMMY, SAFA);
		EnviarPaquete(cpu_libre->socket, paquete);
		log_header(logger, paquete, "Dummy %d ejecutando en cpu %d", dtb_exec->gdtPID, cpu_libre->socket);
		break;
	}
	case GDT:
	{
		paquete->header = cargar_header(tamanio_DTB, ESDTB, SAFA);
		EnviarPaquete(cpu_libre->socket, paquete);
		log_header(logger, paquete, "GDT %d ejecutando en cpu %d", dtb_exec->gdtPID, cpu_libre->socket);
		break;
	}
	}
	dtb_actualizar(dtb_exec, lista_listos, lista_ejecutando, dtb_exec->PC, DTB_EJECUTANDO, cpu_libre->socket);
	sem_post(&sem_dummy);
	free(paquete->Payload);
	free(paquete);
}

void ejecutar_primer_dtb_prioridad()
{
	if(list_is_empty(lista_prioridad) || !hay_cpu_libre())
		return;

	t_cpu *cpu_libre = list_find(lista_cpu, esta_libre_cpu);
	cpu_libre->estado = CPU_OCUPADA;
	DTB *dtb = list_get(lista_prioridad, 0);
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

	Paquete *paquete = malloc(sizeof(Paquete));
	int tamanio_dtb = 0;
	paquete->Payload = DTB_serializar(dtb, &tamanio_dtb);
	paquete->Payload = realloc(paquete->Payload, tamanio_dtb + sizeof(u_int32_t));
	memcpy(paquete->Payload + tamanio_dtb, &info_dtb->quantum_faltante, sizeof(u_int32_t));
	paquete->header = cargar_header(tamanio_dtb + sizeof(u_int32_t), ESDTBQUANTUM, SAFA);
	EnviarPaquete(cpu_libre->socket, paquete);
	log_header(logger, paquete, "Envie desde la cola prioritaria al GDT %d a la CPU %d", dtb->gdtPID, cpu_libre->socket);


	free(paquete->Payload);
	free(paquete);
	info_dtb->quantum_faltante = 0;

	dtb_actualizar(dtb, lista_prioridad, lista_ejecutando, dtb->PC, DTB_EJECUTANDO, cpu_libre->socket);
}

//Funciones asociadas a DTB
DTB *dtb_crear(u_int32_t pid, char *path, int flag_inicializacion)
{
	DTB *dtb_nuevo = malloc(sizeof(DTB));
	dtb_nuevo->flagInicializacion = flag_inicializacion;
	dtb_nuevo->PC = 1;
	dtb_nuevo->entrada_salidas = 0;
	dtb_nuevo->archivosAbiertos = list_create();
	DTB_agregar_archivo(dtb_nuevo, 0, 0, 0, path);

	switch (flag_inicializacion)
	{
	case DUMMY:
	{
		dtb_nuevo->gdtPID = pid;
		list_add(lista_listos, dtb_nuevo);
		sem_post(&sem_listos);
		log_info(logger, "Dummy del GDT %d creado correctamente", dtb_nuevo->gdtPID);
		break;
	}
	case GDT:
	{
		dtb_nuevo->gdtPID = ++numero_pid;
		info_dtb_crear(dtb_nuevo->gdtPID);
		list_add(lista_nuevos, dtb_nuevo);
		log_info(logger, "GDT %d creado correctamente", dtb_nuevo->gdtPID);
		procesos_actuales++;
		break;
	}
	default:
		log_debug(logger, "flag_inicializacion invalida");
	}

	return dtb_nuevo;
}

DTB_info *info_dtb_crear(u_int32_t pid)
{
	DTB_info *info_dtb = malloc(sizeof(DTB_info));
	info_dtb->gdtPID = pid;
	info_dtb->estado = DTB_NUEVO;
	info_dtb->socket_cpu = 0;
	info_dtb->tiempo_ini = 0;
	info_dtb->tiempo_fin = 0;
	info_dtb->tiempo_respuesta = 0;
	info_dtb->kill = false;
	info_dtb->recursos_asignados = list_create();
	info_dtb->quantum_faltante = 0;
	info_dtb->sentencias_en_nuevo = 0;
	info_dtb->sentencias_hasta_finalizar = 0;
	list_add(lista_info_dtb, info_dtb);

	return info_dtb;
}

DTB *dtb_actualizar(DTB *dtb, t_list *source, t_list *dest, u_int32_t pc, Estado estado, int socket)
{
	sem_wait(&actualizar);
	sem_post(&sem_listos);
	if (dtb_encuentra(source, dtb->gdtPID, dtb->flagInicializacion) != NULL)
		dtb_remueve(source, dtb->gdtPID, dtb->flagInicializacion);

	else
		log_debug(logger, "NO SE ENCONTRO al DTB %d en la lista de origen", dtb->gdtPID);

	list_add(dest, dtb);
	dtb->PC = pc;

	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

	if (dtb->flagInicializacion)
	{
		switch (estado)
		{
		case DTB_FINALIZADO:
		{
			procesos_finalizados++;
			log_info(logger, "GDT %d %s->%s", dtb->gdtPID, Estados[info_dtb->estado], Estados[estado]);
			advertencia();
			break;
		}
		case DTB_LISTO:
		{
			if(info_dtb->quantum_faltante)
				log_info(logger, "GDT %d %s->Listo Prioridad", dtb->gdtPID, Estados[info_dtb->estado]);
			else
				log_info(logger, "GDT %d %s->%s", dtb->gdtPID, Estados[info_dtb->estado], Estados[estado]);
			break;	
		}
		default:
		{
			log_info(logger, "GDT %d %s->%s", dtb->gdtPID, Estados[info_dtb->estado], Estados[estado]);
			break;
		}
		}
		info_dtb_actualizar(estado, socket, info_dtb);
	}
	else
		log_info(logger, "Dummy %d->%s", dtb->gdtPID, Estados[estado]);

	sem_post(&actualizar);
	return dtb;
}

DTB_info *info_dtb_actualizar(Estado estado, int socket, DTB_info *info_dtb)
{
	info_dtb->socket_cpu = socket;
	info_dtb->estado = estado;
	return info_dtb;
}

bool dtb_coincide_pid(void *dtb, u_int32_t pid, u_int32_t flag)
{
	return ((DTB *)dtb)->gdtPID == pid && ((DTB *)dtb)->flagInicializacion == flag;
}

bool info_coincide_pid(u_int32_t pid, void *info_dtb)
{
	return ((DTB_info *)info_dtb)->gdtPID == pid;
}

DTB_info *info_asociada_a_pid(u_int32_t pid) //No hace lo mismo que la de arriba? COMPLETAR
{
	bool compara_con_info(void *info_dtb)
	{
		return info_coincide_pid(pid, info_dtb);
	}
	return list_find(lista_info_dtb, compara_con_info);
}

DTB *dtb_encuentra(t_list *lista, u_int32_t pid, u_int32_t flag)
{
	bool compara_con_dtb(void *dtb)
	{
		return dtb_coincide_pid(dtb, pid, flag);
	}
	return list_find(lista, compara_con_dtb);
}

DTB *dtb_remueve(t_list *lista, u_int32_t pid, u_int32_t flag)
{
	bool compara_con_dtb(void *dtb)
	{
		return dtb_coincide_pid(dtb, pid, flag);
	}
	return list_remove_by_condition(lista, compara_con_dtb);
}
// va sin parentesis compara_con_dtb porque tiene que llamar al puntero a la funcion despues de que hacer list_find

void liberar_cpu(int socket)
{
	t_cpu *cpu_actual = cpu_con_socket(socket);
	cpu_actual->estado = CPU_LIBRE;
	cpu_actual->dtb_ejecutados++;
	log_info(logger, "Libere la cpu %d", cpu_actual->socket);
}

bool dtb_coincide_socket(int socket, void *dtb)
{
	DTB_info *info_dtb = info_asociada_a_pid(((DTB *)dtb)->gdtPID);
	return info_dtb->socket_cpu == socket;
}

bool cpu_coincide_socket(int socket, void *cpu)
{
	return ((t_cpu *)cpu)->socket == socket;
}

t_cpu *cpu_con_socket(int socket)
{
	bool compara_cpu(void *cpu)
	{
		return cpu_coincide_socket(socket, cpu);
	}
	return list_find(lista_cpu, compara_cpu);
}

bool esta_libre_cpu(void *cpu)
{
	t_cpu *cpu_recv = (t_cpu *)cpu;
	return (cpu_recv->estado == CPU_LIBRE);
}

bool hay_cpu_libre()
{
	return list_any_satisfy(lista_cpu, esta_libre_cpu);
}

//

DTB *dtb_buscar_en_todos_lados(u_int32_t pid, DTB_info **info_dtb, t_list **lista_actual)
{
	DTB *dtb_encontrado;
	for (u_int32_t i = 0; i < (list_size(lista_estados)); i++)
	{
		*lista_actual = list_get(lista_estados, i);
		dtb_encontrado = dtb_encuentra(*lista_actual, pid, 1);
		if (dtb_encontrado != NULL)
		{
			*info_dtb = info_asociada_a_pid(dtb_encontrado->gdtPID); //Devuelve el DTB que se guarda localmente en planificador
			break;
		}
	}

	if (dtb_encontrado == NULL && pid <= numero_pid)
		log_info(logger, "El GDT %d ya fue finalizado y removido de memoria.\nVer archivo DTB_finalizados.log",
			   pid);
	else if (dtb_encontrado == NULL)
		log_info(logger, "Nunca ingreso al sistema un GDT con PID %d", pid);

	return dtb_encontrado;
}

//Funciones de Consola
void ejecutar(char *path)
{
	dtb_crear(0, path, GDT);
	sem_post(&sem_ejecutar);
}

// Status
void status()
{
	for (int i = 0; i < (list_size(lista_estados)); i++)
	{
		t_list *lista_mostrar = list_get(lista_estados, i);
		log_info(logger, "Cola de %ss:", Estados[i]);
		if (!list_is_empty(lista_mostrar))
		{
			contar_dummys_y_gdt(lista_mostrar);
			list_iterate(lista_mostrar, dtb_imprimir_basico);
		}
	}
	int multip_actual = list_count_satisfying(lista_listos, es_gdt);
	multip_actual += list_count_satisfying(lista_ejecutando, es_gdt);
	multip_actual += list_count_satisfying(lista_bloqueados, es_gdt);
	log_info(logger, "Procesos en memoria: %d", multip_actual);
	log_info(logger, "Grado de multiprogramacion es: %d", MULTIPROGRAMACION);
	return;
}

void gdt_status(u_int32_t pid)
{
	t_list *lista_actual;
	DTB_info *info_dtb;
	DTB *dtb_status = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);

	if (dtb_status != NULL)
		mostrar_proceso(dtb_status);
}

void contar_dummys_y_gdt(t_list *lista)
{
	int cant_dummys = list_count_satisfying(lista, es_dummy);
	int cant_gdt = list_size(lista) - cant_dummys;
	if (cant_dummys != 0)
		log_info(logger, "Cantidad de DUMMYS: %i", cant_dummys);
	log_info(logger, "Cantidad de GDTs: %i", cant_gdt);
}

void dtb_imprimir_basico(void *_dtb)
{
	DTB *dtb = (DTB *)_dtb;
	switch (dtb->flagInicializacion)
	{
	case DUMMY:
	{
		log_info(logger, "DTB Dummy asociado a GDT %d", dtb->gdtPID);
		break;
	}
	case GDT:
	{
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
		log_info(logger, "PID: %i\tEscriptorio: %s", dtb->gdtPID, escriptorio->path);
		break;
	}
	}
}

void dtb_imprimir_polenta(void *_dtb)
{
	DTB *dtb = (DTB *)_dtb;
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	log_info(logger, "Estado: %s", Estados[info_dtb->estado]);
	log_info(logger, "Program Counter: %i", dtb->PC);
	log_info(logger, "Ultima CPU: %i", info_dtb->socket_cpu);
	log_info(logger, "Tiempo de respuesta: %.2f", info_dtb->tiempo_respuesta);
	log_info(logger, "%s", (info_dtb->kill) ? "Proceso finalizado por el usuario" : "");
	log_info(logger, "Archivos Abiertos: %d", list_size(dtb->archivosAbiertos) - 1);
	// Muestra archivos desde el indice 1 (todos menos el escriptorio)
	int i;
	for (i = 0; i < list_size(dtb->archivosAbiertos); i++)
	{
		ArchivoAbierto *archivo = list_get(dtb->archivosAbiertos, i);
		mostrar_archivo(archivo, i);
	}

	// Muestra la cantidad de recursos que tiene retenidos y que recurso y cuantas instancias del mismo tiene
	log_info(logger, "Recursos retenidos por el proceso: %d", list_size(info_dtb->recursos_asignados));
	if (!list_is_empty(info_dtb->recursos_asignados))
	{
		for (int j = 0; j < list_size(info_dtb->recursos_asignados); j++)
		{
			t_recurso_asignado *recurso_asignado = list_get(info_dtb->recursos_asignados, j);
			log_info(logger, "Recurso %d: %s\t %d instancias",
				   j, recurso_asignado->recurso->id, recurso_asignado->instancias);
		}
	}
}

void mostrar_proceso(void *_dtb)
{
	DTB *dtb = (DTB *)_dtb;
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	switch (info_dtb->estado)
	{
	case DTB_NUEVO:
	{
		dtb_imprimir_basico(dtb);
		log_info(logger, "Estado: %s", Estados[info_dtb->estado]);
		break;
	}
	case DTB_FINALIZADO:
	{
		if (dtb->PC == 1 && !info_dtb->kill)
		{
			dtb_imprimir_basico(dtb);
			log_info(logger, "Estado: %s", Estados[info_dtb->estado]);
			if (info_dtb->kill)
				log_info(logger, "Proceso finalizado por el usuario antes de empezar a ejecutar");
			else
				log_info(logger, "Fallo la carga de este DTB en memoria");
		}
		else
		{
			dtb_imprimir_basico(dtb);
			dtb_imprimir_polenta(dtb);
		}
		break;
	}
	default:
	{
		dtb_imprimir_basico(dtb);
		dtb_imprimir_polenta(dtb);
		break;
	}
	}
}

void mostrar_archivo(void *_archivo, int index) // queda en void *_archivo por si volvemos a list_iterate
{
	ArchivoAbierto *archivo = (ArchivoAbierto *)_archivo;
	if(index == 0)
		log_info(logger, "Escriptorio: %s", archivo->path);
	else
	{	
		log_info(logger, "Archivo %d:\t"
		"Directorio: %s",
		// Agregar si se agregan campos a ArchivoAbierto
		index, archivo->path);
	}

	if (archivo->cantLineas)
		log_info(logger, "Cantidad de lineas: %i", archivo->cantLineas);
	mostrar_posicion(archivo);
}

void mostrar_posicion(ArchivoAbierto *archivo)
{
	log_info(logger, "Posicion en memoria:\t"
		   "Segmento: %d\t"
		   "Pagina: %d",
		   archivo->segmento, archivo->pagina);
}

bool es_dummy(void *dtb)
{
	return (((DTB *)dtb)->flagInicializacion == DUMMY);
}

bool es_gdt(void *dtb)
{
	return (((DTB *)dtb)->flagInicializacion == GDT);
}

// Finalizar
void finalizar(u_int32_t pid)
{
	t_list *lista_actual;
	DTB_info *info_dtb;

	DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);
	/* lista_actual queda modificada con la lista donde está el pid que busco */
	if (dtb != NULL)
		manejar_finalizar(dtb, pid, info_dtb, lista_actual);
}

void manejar_finalizar(DTB *dtb, u_int32_t pid, DTB_info *info_dtb, t_list *lista_actual)
{
	info_dtb->kill = true;
	switch (info_dtb->estado)
	{
	case DTB_NUEVO:
	{
		log_info(logger, "El GDT con PID %d esta en nuevos", pid);
		if (!dummy_creado(dtb))
		{
			loggear_finalizacion(dtb, info_dtb);
			dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
		}
		else if (dtb_encuentra(lista_listos, pid, DUMMY) != NULL)
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
		log_info(logger, "El GDT con PID %d esta listo", pid);
		dtb_finalizar(dtb, lista_actual, pid, dtb->PC);
		break;
	}
	case DTB_EJECUTANDO:
	{
		log_info(logger, "El GDT con PID %d esta ejecutando", pid);
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
		log_info(logger, "El GDT con PID %d ya fue finalizado", pid);
		break;
	}
	default:
	{
		log_info(logger, "El proceso con PID %d no esta en ninguna cola", pid);
		break;
	}
	}
}

void manejar_finalizar_bloqueado(DTB *dtb, u_int32_t pid, DTB_info *info_dtb, t_list *lista_actual)
{
	t_recurso *recurso = recurso_bloqueando_pid(pid);
	if (recurso != NULL)
	{
		log_debug(logger, "El GDT %d esta bloqueado esperando por el recurso %s", pid, recurso->id);
		bool coincide_pid(void *_pid_recurso)
		{
			Pid *pidb = (Pid *)_pid_recurso;
			return pidb->pid == pid;
		}
		list_remove_by_condition(recurso->pid_bloqueados, coincide_pid);

		dtb_finalizar(dtb, lista_actual, pid, dtb->PC);
		log_debug(logger, "GDT %d finalizado. Recurso %s liberado", dtb->gdtPID, recurso->id);

	}
	else
		log_debug(logger, "El GDT %d esta bloqueado esperando una E/S\nFinalizara cuando termine", pid);
}

t_recurso *recurso_bloqueando_pid(u_int32_t pid)
{
	bool tiene_el_pid(void *_recurso)
	{
		bool coincide_pid(void *_pid_recurso)
		{
			Pid *pidb = (Pid *)_pid_recurso;
			return pidb->pid == pid;
		}
		t_recurso *recurso = (t_recurso *)_recurso;
		return list_any_satisfy(recurso->pid_bloqueados, coincide_pid);
	}

	return list_find(lista_recursos_global, tiene_el_pid);
}

void dtb_finalizar(DTB *dtb, t_list *lista_actual, u_int32_t pid, u_int32_t pc)
{
	t_list *actual = lista_actual;
	DTB_info *info_dtb = info_asociada_a_pid(pid);

	if(dtb == NULL)
	{
		dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &actual);
		pc = dtb->PC;
		log_debug(logger, "No se encontro al dtb %d en la lista que deberia estar", dtb->gdtPID);
	}

	loggear_finalizacion(dtb, info_dtb);
	limpiar_recursos(info_dtb);
	dtb_actualizar(dtb, actual, lista_finalizados, pc, DTB_FINALIZADO, info_dtb->socket_cpu);
	enviar_finalizar_dam(dtb->gdtPID);
}

void loggear_finalizacion(DTB *dtb, DTB_info *info_dtb)
{
	log_info(logger_fin, "El proceso con PID %d fue finalizado", dtb->gdtPID);
	log_info(logger_fin, "Tiempo de respuesta: %.2f milisegundos", info_dtb->tiempo_respuesta);
	log_info(logger_fin, "Proceso Finalizado por Usuario: %s\n", (info_dtb->kill) ? "Si" : "No");
	log_info(logger_fin, "Ultimo estado: %s", Estados[info_dtb->estado]);

	if (dtb->PC > 1)
	{
		if (info_dtb->socket_cpu)
			log_info(logger_fin, "Ultima cpu usada: %d", info_dtb->socket_cpu);

		log_info(logger_fin, "Sentencias de E/S: %d", dtb->entrada_salidas);
	}
	else if (dtb->PC == 1 && !info_dtb->kill)
		log_info(logger_fin, "El proceso nunca pudo ser cargado en memoria");

	log_info(logger_fin, "Archivos abiertos: %d", list_size(dtb->archivosAbiertos));
	for (int i = 0; i < list_size(dtb->archivosAbiertos); i++)
	{
		ArchivoAbierto *archivo;
		archivo = list_get(dtb->archivosAbiertos, i);
		if (i == 0)
			log_info(logger_fin, "Path Escriptorio: %s", archivo->path);
		else
			log_info(logger_fin, "Archivo n° %i: %s", i, archivo->path);
	}

	log_info(logger_fin, "Recursos retenidos: %d", list_size(info_dtb->recursos_asignados));
	for (int i = 0; i < list_size(info_dtb->recursos_asignados); i++)
	{
		t_recurso_asignado *recurso;
		recurso = list_get(info_dtb->recursos_asignados, i);
		log_info(logger_fin, "Recurso retenido nro %i: %s", i + 1, recurso->recurso->id);
	}
}

void enviar_finalizar_dam(u_int32_t pid)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	DTB_info *info_dtb;
	t_list *actual;
	DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &actual);
	ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
	Posicion *posicion = generar_posicion(dtb, escriptorio, 0);
	log_posicion(logger, posicion, "Posicion a finalizar");
	
	int tam_pos = 0;
	void *posicion_serializada = serializar_posicion(posicion, &tam_pos);
	paquete->header = cargar_header(tam_pos, FINALIZAR, SAFA);
	paquete->Payload = malloc(paquete->header.tamPayload);
	memcpy(paquete->Payload, posicion_serializada, tam_pos);
	sem_wait(&sem_dam);
	EnviarPaquete(socket_diego, paquete);
	sem_post(&sem_dam);
	log_header(logger, paquete, "Envie finalizar GDT %d a Diego", posicion->pid);
	free(posicion_serializada);
	free(posicion);
	free(paquete->Payload);
	free(paquete);
}

void enviar_finalizar_cpu(u_int32_t pid, int socket_cpu)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->Payload = malloc(sizeof(u_int32_t));
	memcpy(paquete->Payload, &pid, sizeof(u_int32_t));
	paquete->header = cargar_header(sizeof(u_int32_t), FINALIZAR, SAFA);
	EnviarPaquete(socket_cpu, paquete);
	log_header(logger, paquete, "Envie finalizar GDT %d a CPU %d", pid, socket_cpu);
	free(paquete->Payload);
	free(paquete);
}

// Recursos
t_recurso *recurso_crear(char *id_recurso, int valor_inicial)
{
	t_recurso *recurso = malloc(sizeof(t_recurso));
	recurso->id = malloc(strlen(id_recurso) + 1);
	strcpy(recurso->id, id_recurso);
	recurso->semaforo = valor_inicial;
	recurso->pid_bloqueados = list_create();

	log_recurso(logger, recurso, "Cree el recurso:");

	list_add(lista_recursos_global, recurso);

	return recurso;
}

void recurso_liberar(void *_recurso)
{
	t_recurso *recurso = (t_recurso *)_recurso;
	free(recurso->id);
	list_destroy_and_destroy_elements(recurso->pid_bloqueados, free);
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
	for (int i = 0; i < list_size(info_dtb->recursos_asignados); i++)
	{
		t_recurso_asignado *recurso_asignado = list_get(info_dtb->recursos_asignados, i);
		forzar_signal(recurso_asignado);
	}
	list_clean_and_destroy_elements(info_dtb->recursos_asignados, free);
}

void forzar_signal(t_recurso_asignado *recurso_asignado)
{
	while (recurso_asignado->instancias)
	{
		dtb_signal(recurso_asignado->recurso);
		recurso_asignado->instancias--;
	}
}

void dtb_signal(t_recurso *recurso)
{
	log_debug(logger, "Tengo %d procesos bloqueados", list_size(recurso->pid_bloqueados));
	Pid *pid_desbloqueado = list_remove(recurso->pid_bloqueados, 0);
	log_debug(logger, "POST-REMOVE Tengo %d procesos bloqueados", list_size(recurso->pid_bloqueados));
	
	t_list *actual;
	DTB_info *info_dtb;
	actual = lista_bloqueados;

	if (pid_desbloqueado != NULL)
	{
		int pid = pid_desbloqueado->pid;
		DTB *dtb = dtb_encuentra(actual, pid, GDT);
		if(dtb == NULL)
		{
			dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &actual);
			log_debug(logger, "no lo encontro en la lista de bloqueados a GDT %d", dtb->gdtPID);
		}
		log_debug(logger, "Encontro al DTB %d bloqueado por recurso", dtb->gdtPID);
		info_dtb = info_asociada_a_pid(dtb->gdtPID);
		recurso_asignar_a_pid(recurso, dtb->gdtPID);
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
		if(dtb->PC == escriptorio->cantLineas)
		{
			log_debug(logger, "El GDT %d desbloqueado va a finalizarse", dtb->gdtPID);
			dtb_finalizar(dtb, lista_bloqueados, dtb->gdtPID, dtb->PC);
			return;
		}
		dtb_actualizar(dtb, actual, lista_listos, dtb->PC, DTB_LISTO, info_dtb->socket_cpu);
	}
	else
		recurso->semaforo++;

	log_recurso(logger, recurso, "Recurso post signal:");
}

void recurso_asignar_a_pid(t_recurso *recurso, u_int32_t pid)
{
	DTB_info *info_dtb = info_asociada_a_pid(pid);
	t_recurso_asignado *rec_asignado = NULL;

	bool esta_asignado(void *_recurso_asignado)
	{
		t_recurso_asignado *recurso_asignado = (t_recurso_asignado *)_recurso_asignado;
		return recurso_coincide_id(recurso_asignado->recurso, recurso->id);
	}

	if (list_size(info_dtb->recursos_asignados) > 0)
	{
		rec_asignado = list_find(info_dtb->recursos_asignados, esta_asignado);
	}

	if (rec_asignado == NULL)
	{
		rec_asignado = malloc(sizeof(t_recurso_asignado));
		rec_asignado->recurso = recurso;
		rec_asignado->instancias = 1;
		list_add(info_dtb->recursos_asignados, rec_asignado);
		return;
	}

	rec_asignado->instancias++;
}

clock_t medir_tiempo ()
{
	clock_t t_inst;
	t_inst = clock();
	return t_inst;
}

double calcular_RT(clock_t t_ini_rcv, clock_t t_fin_rcv)
{
	double rt; //response time
	clock_t t_ini;
	clock_t t_fin;

	t_ini = t_ini_rcv;
	t_fin = t_fin_rcv;
	rt = ((double)(t_fin - t_ini) / CLOCKS_PER_SEC) * 1000;
	trt += rt;
	return rt;
}

//COMO SE USAN: EN LISTOS se toma el inicio de tiempo y en la primera ejecucion se toma el final
/*	info_dtb->tiempo_ini = medir_tiempo();
	info_dtb->tiempo_fin = medir_tiempo();
	info_dtb->tiempo_respuesta = calcular_RT(info_dtb->tiempo_ini, info_dtb->tiempo_fin);
*/

void gdt_metricas(u_int32_t pid)
{
	metricas();
	DTB_info *info_dtb;
	t_list *actual;
	DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &actual);
	if (dtb != NULL)
	{
		log_info(logger, "Estadisticas del GDT %d", pid);
		log_info(logger, "Sentencias totales esperadas en nuevo: %.2f", info_dtb->sentencias_en_nuevo);
		log_info(logger, "Entradas/Salidas: %d", dtb->entrada_salidas);
		log_info(logger, "Sentencias totales hasta finalizar: %.2f", info_dtb->sentencias_hasta_finalizar);
		log_info(logger, "Tiempo de respuesta: %.2f", info_dtb->tiempo_respuesta);
	}
}

void metricas()
{
	log_info(logger, "Estadisticas de la fecha");
	if (procesos_actuales)
	{
		double sentencias_promedio_diego = sentencias_globales_del_diego / (double) procesos_actuales;
		log_info(logger, "Sentencias ejecutadas promedio del sistema que usaron a \"El Diego\": %.2f", sentencias_promedio_diego);
	}
	else
		log_info(logger, "Todavia no empezo el campeonato (no hay procesos en el sistema)");
	
	if (procesos_finalizados)
	{
		double sentencias_promedio_hasta_finalizar = calcular_sentencias_promedio_hasta_finalizar();
		log_info(logger, "Sentencias ejecutadas promedio del sistema para que un dtb termine en la cola de EXIT: %.2f", sentencias_promedio_hasta_finalizar);
	}
	else
		log_info(logger, "Ningun GDT finalizo hasta el momento");
	
	if (sentencias_totales)
	{
		double porcentaje_al_diego = 100 * (sentencias_globales_del_diego / sentencias_totales);
		log_info(logger, "Porcentaje de las sentencias ejecutadas promedio que fueron a \"El Diego\": %.2f %%", porcentaje_al_diego);
	}
	else
		log_info(logger, "Ninguna sentencia fue ejecutada hasta el momento");

	if(procesos_actuales)
		average_rt = trt / (double) procesos_actuales;

	log_info(logger, "Tiempo de respuesta promedio del sistema: %.2f milisegundos", average_rt);
}

double calcular_sentencias_promedio_hasta_finalizar()
{
	double acum = 0;
	void _sumar_sentencias_hasta_finalizar(void *_info_dtb)
	{
		acum += ((DTB_info *)_info_dtb)->sentencias_hasta_finalizar;
	}

	t_list *finalizados = list_filter(lista_info_dtb, ya_finalizo);

	if (finalizados == NULL)
		return 0;
	
	list_iterate(finalizados, _sumar_sentencias_hasta_finalizar);
	
	return acum / procesos_finalizados;
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
	{
		info_liberar(dtb);
		log_info(logger, "Se libero de memoria del DTB %d", ((DTB *)dtb)->gdtPID);
	}
	else
		log_info(logger, "Se libero de memoria el Dummy del GDT %d", ((DTB *)dtb)->gdtPID);

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

void info_liberar_completo(void *_info_dtb)
{
	DTB_info *info_dtb = (DTB_info *)_info_dtb;
	list_clean_and_destroy_elements(info_dtb->recursos_asignados, free);
	free(info_dtb);
}

void advertencia()
{
	procesos_a_esperar--;
	char *c = malloc(10);
	int procesos_eliminar;
	if (procesos_finalizados > 20 && procesos_a_esperar <= 0) //Te lo puse en 1 para poder probarlo. Va hardcodeado en 30?
	{
		printf("\nHay mas de 20 procesos finalizados, seleccione una opcion:\n"
			   "1: Eliminar todos los procesos.\n"
			   "2: Elegir la cantidad de procesos a eliminar\n"
			   "3: Continuar sin hacer nada\n"
			   ">> "); // Le puse esto para que simule seguir siendo la consola
		scanf("%s", c);
		int d = atoi(c);
		switch (d)
		{
		case 1:
		{
			liberar_memoria();
			break;
		}
		case 2:
		{
			printf("Ingrese la cantidad de procesos a eliminar: ");
			scanf("%d", &procesos_eliminar);
			liberar_parte_de_memoria(procesos_eliminar);
			break;
		}
		case 3:
		{
			procesos_a_esperar = 5;
			break;
		}
		default:
			break;
		}
	}
	free(c);
}

void liberar_memoria()
{
	int procesos_liberados;
	procesos_liberados = list_size(lista_finalizados);

	list_iterate(lista_finalizados, restar_sentencias_globales);

	list_clean_and_destroy_elements(lista_finalizados, dtb_liberar);
	log_info(logger, "Por consola, se liberaron de memoria %i procesos de la lista de finalizados", procesos_liberados);
	procesos_finalizados -= procesos_liberados;
	procesos_actuales -= procesos_liberados;
	trt = 0;
}

void liberar_parte_de_memoria(int procesos_eliminar)
{
	if (procesos_eliminar < list_size(lista_finalizados))
	{
		procesos_finalizados -= procesos_eliminar;
		procesos_actuales -= procesos_eliminar;
		t_list *liberar = list_take(lista_finalizados, procesos_eliminar);
		list_iterate(liberar, restar_sentencias_globales);
		list_iterate(liberar, restar_trespuesta);
		list_destroy(liberar);
		for (int i = 0; i < procesos_eliminar; i++)
		{
			list_remove_and_destroy_element(lista_finalizados, 0, dtb_liberar);
		}
		log_info(logger, "Se finalizaron los primeros %i procesos de la lista de finalizados", procesos_eliminar);
	}
	else
	{
		if (procesos_eliminar > list_size(lista_finalizados))
			log_debug(logger, "Solo hay %d procesos en la lista de finalizados\n", procesos_finalizados);
		liberar_memoria();
	}
}

void restar_sentencias_globales(void *_dtb)
{
	DTB *dtb = (DTB *)_dtb;

	sentencias_globales_del_diego -= dtb->entrada_salidas;
	ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
	sentencias_totales -= (dtb->PC - 1);
	log_debug(logger, "Sentencias totales %.2f", sentencias_totales);
	log_debug(logger, "Sentencias globales al diego: %.2f", sentencias_globales_del_diego);
}

void restar_trespuesta(void *_dtb)
{
	DTB *dtb = (DTB *)_dtb;
	DTB_info *info = info_asociada_a_pid(dtb->gdtPID);
	trt -= info->tiempo_respuesta;
	log_debug(logger, "trt: %.2f", trt);
}

void log_info_dtb(t_log *logger, DTB_info *info_dtb, const char* _contexto, ...)
{
    va_list arguments;
    va_start(arguments, _contexto);
    char *contexto = string_from_vformat(_contexto, arguments);
    log_info(logger, "%s", contexto);

    va_end(arguments);	
    free(contexto);

    log_debug(logger, "info_dtb->estado: %s", Estados[info_dtb->estado]);
    log_debug(logger, "info_dtb->socket_cpu: %d", info_dtb->socket_cpu);
    log_debug(logger, "info_dtb->tiempo_respuesta: %.2f", info_dtb->tiempo_respuesta);
	if(info_dtb->kill)
    	log_debug(logger, "Finalizo por consola");

    int i;
    for(i = 0; i < list_size(info_dtb->recursos_asignados); i++)
    {
        t_recurso_asignado *rec_asignado = list_get(info_dtb->recursos_asignados, i);
        log_recurso(logger, rec_asignado->recurso, "info_dtb->recursos_asignados");
        log_debug(logger, "Instancias: %d", rec_asignado->instancias);
    }
    if(info_dtb->quantum_faltante)
        log_debug(logger, "info_dtb->quantum_faltante: %d", info_dtb->quantum_faltante);
    
    log_debug(logger, "info_dtb->sentencias_en_nuevo: %d", info_dtb->sentencias_en_nuevo);
    log_debug(logger, "info_dtb->sentencias_hasta_finalizar: %d", info_dtb->sentencias_hasta_finalizar);
}

void log_recurso(t_log *logger, t_recurso *recurso, const char *_contexto, ...)
{
    va_list arguments;
    va_start(arguments, _contexto);
    char *contexto = string_from_vformat(_contexto, arguments);
    log_info(logger, "%s", contexto);

    va_end(arguments);	
    free(contexto);

    log_debug(logger, "recurso->id: %s", recurso->id);
    log_debug(logger, "recurso->semaforo: %d", recurso->semaforo);
    
    int i;
    log_debug(logger, "recurso->pid_bloqueados: %d", list_size(recurso->pid_bloqueados));
    for(i = 0; i < list_size(recurso->pid_bloqueados); i++)
    {
        Pid *pidb = list_get(recurso->pid_bloqueados, i);
        log_debug(logger, "Bloqueado nro %d: GDT %d", i + 1, pidb->pid);
    }
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
