#include "safa.h"

int transfer_size = 0;

/*Creación de Logger*/
void crear_loggers()
{
	logger = log_create("logs/safa.log", "safa", true, LOG_LEVEL_DEBUG);
	logger_fin = log_create ("logs/DTB_finalizados.log", "safa", true, LOG_LEVEL_DEBUG);
	log_info(logger, "Iniciando SAFA.log");
	log_info(logger_fin, "INFORMACION DE DTBs FINALIZADOS");
}

void obtener_valores_archivo_configuracion()
{
	t_config *arch = config_create("./SAFA.config");
	IP = string_duplicate(config_get_string_value(arch, "IP"));
	PUERTO = config_get_int_value(arch, "PUERTO");
	ALGORITMO_PLANIFICACION = string_duplicate(config_get_string_value(arch, "ALGORITMO"));
	QUANTUM = config_get_int_value(arch, "QUANTUM");
	MULTIPROGRAMACION = config_get_int_value(arch, "MULTIPROGRAMACION");
	RETARDO_PLANIF = config_get_int_value(arch, "RETARDO_PLANIF");
	config_destroy(arch);
}

void imprimir_archivo_configuracion()
{
	log_info(logger, "CONFIGURACION");
	log_info(logger, "IP: %s", IP);
	log_info(logger, "Puerto: %d", PUERTO);
	log_info(logger, "Algoritmo de planificacion: %s", ALGORITMO_PLANIFICACION);
	log_info(logger, "Quantum: %d", QUANTUM);
	log_info(logger, "Multiprogramacion: %d", MULTIPROGRAMACION);
	log_info(logger, "Retardo de planificacion: %dms", RETARDO_PLANIF);
}

void inicializar_variables() {
	crear_listas();
	llenar_lista_estados();
	sem_init(&mutex_handshake_diego, 0, 0);
	sem_init(&mutex_handshake_cpu, 0, 0);
	sem_init(&sem_ejecutar, 0, 0);
	sem_init(&sem_multiprogramacion, 0, MULTIPROGRAMACION);
	sem_init(&sem_listos, 0, 0);
	sem_init(&sem_metricas, 0, 1);
	sem_init(&sem_dummy, 0, 1);
	sem_init(&sem_dam, 0, 1);
	sem_init(&actualizar, 0, 1);

	int a = 0;
	sem_getvalue(&sem_multiprogramacion, &a);
	log_info(logger, "sem_multiprogramacion es %d", a);
}

void crear_listas()
{
	lista_cpu = list_create();
	lista_nuevos = list_create();
	lista_listos = list_create();
	lista_ejecutando = list_create();
	lista_bloqueados = list_create();
	lista_finalizados = list_create();
	lista_prioridad = list_create();
	lista_estados = list_create();
	lista_info_dtb = list_create();
	lista_recursos_global = list_create();
}

void llenar_lista_estados()
{
	list_add_in_index(lista_estados, 0, lista_nuevos);
	list_add_in_index(lista_estados, 1, lista_listos);
	list_add_in_index(lista_estados, 2, lista_ejecutando);
	list_add_in_index(lista_estados, 3, lista_bloqueados);
	list_add_in_index(lista_estados, 4, lista_finalizados);
}

void consola()
{
	log_info(logger, "Esperando que conecte el diego y al menos 1 CPU\nSAFA esta en estado corrupto");
	sem_wait(&mutex_handshake_diego);
	log_debug(logger, "Handshake con el diego concretado");
	sem_wait(&mutex_handshake_cpu);
	log_debug(logger, "Handshake con un cpu concretado");
	log_info(logger, "SAFA esta en estado operativo");
	log_info(logger, "Puede usar la consola");

	char *linea;
	while (true)
	{
		linea = (char *)readline(">> ");
		if (linea)
			add_history(linea);
		char **parametros = string_split(linea, " ");
		if (parametros[0] == NULL)
		{
			free(parametros);
			continue;
		}
		parseo_consola(parametros[0], parametros[1]);
		free(linea);
		int i;
		for(i = 0; parametros[i] != NULL; i++)
			free(parametros[i]);

		free(parametros);
	}
}

void parseo_consola(char *operacion, char *primer_parametro)
{
	u_int32_t pid = 0;
	if (string_equals_ignore_case(operacion, EJECUTAR))
	{
		if (primer_parametro == NULL)
			log_info(logger, "Falto parametro. Esperaba ejecutar <path>");
		else
		{
			log_info(logger, "EJECUTAR escriptorio %s", primer_parametro);
			ejecutar(primer_parametro);
		}
	}
	else if (string_equals_ignore_case(operacion, STATUS))
	{
		if (primer_parametro != NULL)
		{
			pid = atoi(primer_parametro);
			log_info(logger, "STATUS de GDT %i", pid);
			gdt_status(pid);
		}
		else
		{
			log_info(logger, "STATUS de las colas");
			status();
		}
	}
	else if (string_equals_ignore_case(operacion, FINALIZAR))
	{
		if (primer_parametro == NULL)
		{
			log_info(logger, "Falto parametro. Esperaba finalizar <pid>");
			return;
		}

		pid = atoi(primer_parametro);
		log_info(logger, "FINALIZAR GDT %i", pid);
		finalizar(pid);
	}
	else if (string_equals_ignore_case(operacion, METRICAS))
	{
		if (primer_parametro != NULL)
		{
			pid = atoi(primer_parametro);
			log_info(logger, "METRICAS de GDT %d", pid);
			gdt_metricas(pid);
		}
		else
		{
			log_info(logger, "METRICAS del sistema");
			metricas();
		}
	}
	else if (string_equals_ignore_case(operacion, LIBERAR))
	{
		if (primer_parametro != NULL)
		{
			int procesos_a_liberar = atoi(primer_parametro);
			log_info(logger, "LIBERAR %d GDTs", procesos_a_liberar);
			liberar_parte_de_memoria(procesos_a_liberar);
		}
		else
		{
			log_info(logger, "LIBERAR la cola entera de finalizados (%d GDTs)", list_size(lista_finalizados));
			liberar_memoria();
		}
	}
	else
		log_info(logger, "OPERACION DESCONOCIDA");
}

void accion(void *socket)
{
	int socketFD = *(int *)socket;
	Paquete paquete;
	// void* datos;
	//while que recibe paquetes que envian a safa, de qué socket y el paquete mismo
	//el socket del cliente conectado lo obtiene con la funcion servidorConcurrente en el main

	//El transfer size no puede ser menor al tamanio del header porque sino no se puede saber quien es el emisor
	//while (RecibirPaqueteServidorSafa(socketFD, SAFA, &paquete) > 0)
	while (RecibirDatos(&paquete.header , socketFD, TAMANIOHEADER) > 0)
	{
		log_header(logger, &paquete, "Llego un paquete en el socket %d", socketFD);
		switch (paquete.header.emisor)
		{
		case ELDIEGO:
		{
			if (paquete.header.tipoMensaje != ESHANDSHAKE && paquete.header.tamPayload > 0)
			{
				paquete.Payload = malloc(paquete.header.tamPayload);
				recibir_partes(socketFD, paquete.Payload, paquete.header.tamPayload);
			}

			manejar_paquetes_diego(&paquete, socketFD);
			break;
		}
		case CPU:
		{
			if (paquete.header.tamPayload > 0)
			{
				paquete.Payload = malloc(paquete.header.tamPayload);
				RecibirDatos(paquete.Payload, socketFD, paquete.header.tamPayload);
			}

			manejar_paquetes_CPU(&paquete, socketFD);
			break;
		}
		default:
			log_warning(logger, "No se reconoce el emisor %d", paquete.header.emisor);
			break;
		}

		if(paquete.header.tamPayload > 0)
			free(paquete.Payload);
	}
	// Si sale del while hubo error o desconexion
	manejar_desconexion(socketFD);
	//if (paquete.Payload != NULL && paquete.header.tamPayload > 0)
	//	free(paquete.Payload);

	close(socketFD);
}

void manejar_desconexion(int socket)
{
	if(socket == socket_diego)
	{
		log_error(logger, "Desconexion en el socket %d, donde estaba El Diego", socket_diego);
		log_destroy(logger);
		log_destroy(logger_fin);
		exit(1);
	}
	else
	{
		manejar_desconexion_cpu(socket);
	}		
}

void manejar_desconexion_cpu(int socket)
{
	log_error(logger, "Desconexion de la cpu %d", socket);

	bool _dtb_compara_socket(void *_dtb)
	{
		return dtb_coincide_socket(socket, _dtb);
	}

	DTB *dtb = list_find(lista_ejecutando, _dtb_compara_socket);
	if(dtb != NULL)
	{
		dtb_actualizar(dtb, lista_ejecutando, lista_listos, dtb->PC, DTB_LISTO, socket);
		log_info(logger, "GDT %d desalojado de cpu %d desconectada", dtb->gdtPID, socket);
	}

	bool _compara_cpu(void *_cpu)
	{
		return cpu_coincide_socket(socket, _cpu);
	}
	list_remove_and_destroy_by_condition(lista_cpu, _compara_cpu, free);
	log_info(logger, "Cpu %d removida del sistema", socket);

	if(list_is_empty(lista_cpu))
	{
		log_info(logger, "Se desconecto la ultima cpu del sistema");
		log_info(logger, "Esperando 30 segundos por la conexion de otra");
		log_info(logger, "Si no conecta otra, el sistema finalizara");
		sleep(30);

		if(list_is_empty(lista_cpu))
		{
			log_info(logger, "Sistema terminado correctamente tras la desconexion de todas las CPUs");
			log_info(logger, "Terminando SAFA");

			log_destroy(logger);
			log_destroy(logger_fin);

			exit(1);
		}
	}
	else
		log_info(logger, "Quedan %d cpus", list_size(lista_cpu));

}

void manejar_paquetes_diego(Paquete *paquete, int socketFD)
{
	u_int32_t pid = 0;
	int tam_pid = INTSIZE;
	switch (paquete->header.tipoMensaje)
	{
	case ESHANDSHAKE:
	{
		sem_post(&mutex_handshake_diego);
		socket_diego = socketFD;
		paquete->Payload = malloc(paquete->header.tamPayload);
		RecibirDatos(paquete->Payload, socketFD, INTSIZE);
		log_info(logger, "llegada de el diego en socket %d", socketFD);
		memcpy(&transfer_size, paquete->Payload, INTSIZE);
		enviar_handshake_diego(socketFD);
		break;
	}

	case DUMMY_SUCCESS:
	{
		// Payload contiene pid + ArchivoAbierto
		memcpy(&pid, paquete->Payload, tam_pid);
		DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
		log_info(logger, "GDT %d fue cargado en memoria correctamente", dtb->gdtPID);

		ArchivoAbierto *escriptorio_cargado = DTB_leer_struct_archivo(paquete->Payload, &tam_pid);
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
		escriptorio->cantLineas = escriptorio_cargado->cantLineas;
		escriptorio->segmento = escriptorio_cargado->segmento;
		escriptorio->pagina = escriptorio_cargado->pagina;

		if(verificar_si_murio(dtb, lista_nuevos, pid, dtb->PC))
			break;

		bloquear_dummy(lista_bloqueados, pid);
		notificar_al_plp(dtb->gdtPID);
		log_info(logger, "GDT %d pasa a lista de procesos listos", pid);
		liberar_archivo_abierto(escriptorio_cargado);

		break;
	}
	case DUMMY_FAIL_CARGA:
	{
		sem_post(&sem_multiprogramacion);
		memcpy(&pid, paquete->Payload, tam_pid);
		DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
		log_error(logger, "Fallo la carga en memoria del GDT %d", dtb->gdtPID);
		bloquear_dummy(lista_bloqueados, dtb->gdtPID);
		dtb_actualizar(dtb, lista_nuevos, lista_finalizados, dtb->PC, DTB_FINALIZADO, 0);
		sem_post(&sem_listos);
		break;
	}
	case DUMMY_FAIL_NO_EXISTE:
	{
		sem_post(&sem_multiprogramacion);
		memcpy(&pid, paquete->Payload, tam_pid);
		DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
		log_error(logger, "No existe el escriptorio %s del GDT %d", escriptorio->path, dtb->gdtPID);
		bloquear_dummy(lista_bloqueados, dtb->gdtPID);
		dtb_actualizar(dtb, lista_nuevos, lista_finalizados, dtb->PC, DTB_FINALIZADO, 0);
		sem_post(&sem_listos);
		break;
	}
	case DTB_SUCCESS: //Incluye flush, crear, borrar.
	{
		memcpy(&pid, paquete->Payload, tam_pid);

		DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
		DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
		log_info(logger, "GDT %d realizo la operacion bloqueante correctamente", dtb->gdtPID);

		if(!info_dtb->tiempo_respuesta)
		{
		info_dtb->tiempo_fin = medir_tiempo();
		info_dtb->tiempo_respuesta = calcular_RT(info_dtb->tiempo_ini, info_dtb->tiempo_fin);
		log_debug(logger, "Tiempo respuesta: %.2f", info_dtb->tiempo_respuesta);
		}

		if(dtb->PC == escriptorio->cantLineas)
		{
			dtb_finalizar(dtb, lista_bloqueados, pid, dtb->PC);
			break;
		}

		if(verificar_si_murio(dtb, lista_bloqueados, pid, dtb->PC))
			break;
		
		if(info_dtb->quantum_faltante)
			dtb_actualizar(dtb, lista_bloqueados, lista_prioridad, dtb->PC, DTB_LISTO, info_dtb->socket_cpu);
		else
			dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC, DTB_LISTO, info_dtb->socket_cpu);
		
		break;
	}
	case ARCHIVO_ABIERTO:
	{
		memcpy(&pid, paquete->Payload, tam_pid);
		DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
		DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);

		ArchivoAbierto *archivo = DTB_leer_struct_archivo(paquete->Payload, &tam_pid);
		list_add(dtb->archivosAbiertos, archivo);
		log_archivo(logger, archivo, "GDT %d abrio %s", dtb->gdtPID, archivo->path);
		
		if(!info_dtb->tiempo_respuesta)
		{	
		info_dtb->tiempo_fin = medir_tiempo();
		info_dtb->tiempo_respuesta = calcular_RT(info_dtb->tiempo_ini, info_dtb->tiempo_fin);
		log_debug(logger, "Tiempo respuesta: %.2f", info_dtb->tiempo_respuesta);
		}

		if(dtb->PC == escriptorio->cantLineas)
		{
			dtb_finalizar(dtb, lista_bloqueados, pid, dtb->PC);
			break;
		}

		if(verificar_si_murio(dtb, lista_bloqueados, pid, dtb->PC))
			break;
		
		if(info_dtb->quantum_faltante)
 			dtb_actualizar(dtb, lista_bloqueados, lista_prioridad, dtb->PC, DTB_LISTO, info_dtb->socket_cpu);
 		else
 			dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC, DTB_LISTO, info_dtb->socket_cpu);

		break;
	}
	case DTB_FINALIZAR:
	{
		sem_post(&sem_multiprogramacion);
		memcpy(&pid, paquete->Payload, INTSIZE);
		DTB_info *info_dtb;
		t_list *lista_actual;
		DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);
		log_info(logger, "GDT %d removido de memoria", dtb->gdtPID);
		break;
	}
	default:
		manejar_errores_diego(paquete);
		break;
	}
}

void manejar_errores_diego(Paquete *paquete)
{
	u_int32_t pid = 0;
	memcpy(&pid, paquete->Payload, INTSIZE);
	DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
	dtb_finalizar(dtb, lista_bloqueados, pid, dtb->PC);
	sem_post(&sem_multiprogramacion);
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	if(!info_dtb->tiempo_respuesta)
	{	
		info_dtb->tiempo_fin = medir_tiempo();
		info_dtb->tiempo_respuesta = calcular_RT(info_dtb->tiempo_ini, info_dtb->tiempo_fin);
		log_debug(logger, "Tiempo respuesta: %.2f", info_dtb->tiempo_respuesta);
	}

	switch(paquete->header.tipoMensaje)
	{
	case PATH_INEXISTENTE:
		log_error(logger, "Abrir Error %d: Path inexistente del GDT %d", PATH_INEXISTENTE, pid);
		break;
	case ESPACIO_INSUFICIENTE_ABRIR:
		log_error(logger, "Abrir Error %d: Espacio insuficiente en FM9 del GDT %d", ESPACIO_INSUFICIENTE_ABRIR, pid);
		break;
	case FALLO_DE_SEGMENTO_FLUSH:
		log_error(logger, "Flush Error %d: Fallo de segmento en FM9 del GDT %d", FALLO_DE_SEGMENTO_FLUSH, pid);
		break;
	case ESPACIO_INSUFICIENTE_FLUSH:
		log_error(logger, "Flush Error %d: Espacio insuficiente en MDJ del GDT %d", ESPACIO_INSUFICIENTE_FLUSH, pid);
		break;
	case ARCHIVO_NO_EXISTE_FLUSH:
		log_error(logger, "Flush Error %d: Archivo no existe en MDJ del GDT %d", ARCHIVO_NO_EXISTE_FLUSH, pid);
		break;
	case ARCHIVO_YA_EXISTENTE:
		log_error(logger, "Crear Error %d: Archivo ya existente del GDT %d", ARCHIVO_YA_EXISTENTE, pid);
		break;
	case ESPACIO_INSUFICIENTE_CREAR:
		log_error(logger, "Crear Error %d: Espacio insuficiente en MDJ del GDT %d", ESPACIO_INSUFICIENTE_CREAR, pid);
		break;
	case ARCHIVO_NO_EXISTE:
		log_error(logger, "Borrar Error %d: Archivo inexistente del GDT %d", ARCHIVO_NO_EXISTE, pid);
		break;
	default:
		log_error(logger, "No entendi el mensaje del diego");
		break;
	}
}
void manejar_paquetes_CPU(Paquete *paquete, int socketFD)
{
	log_debug(logger, "Interpreto mensaje de cpu");
	u_int32_t pid = 0;
	u_int32_t pc = 0;
	int desplazamiento = 0;

	switch (paquete->header.tipoMensaje)
	{
	case ESHANDSHAKE:
	{
		t_cpu *cpu_nuevo = malloc(sizeof(t_cpu));
		cpu_nuevo->socket = socketFD;
		cpu_nuevo->estado = CPU_LIBRE;
		cpu_nuevo->dtb_ejecutados = 0;
		list_add(lista_cpu, cpu_nuevo);
		log_info(logger, "llegada cpu con id %i", cpu_nuevo->socket);
		sem_post(&mutex_handshake_cpu);
		enviar_handshake_cpu(socketFD);
		break;
	}
	case DUMMY_BLOQUEA:
	{
		log_debug(logger, "ENTRO A DUMMY BLOQUEA");
		sem_wait(&sem_dummy);
		liberar_cpu(socketFD);
		memcpy(&pid, paquete->Payload, INTSIZE);
		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, DUMMY);
		log_info(logger, "Se ejecuto el dummy de GDT %d", dtb->gdtPID);
		dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, dtb->PC, DTB_BLOQUEADO, socketFD);
		sem_post(&sem_dummy);
		break;
	}
	case DTB_BLOQUEAR:
	{
		liberar_cpu(socketFD);
		deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);

		log_debug(logger, "Bloquea PID %d", pid);

		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		memcpy(&dtb->entrada_salidas, paquete->Payload + desplazamiento, INTSIZE);
		log_debug(logger, "GDT %d actualizo entradas salidas: %d", dtb->gdtPID, dtb->entrada_salidas);

		sentencias_globales_del_diego++;
		metricas_actualizar(dtb, pc);
		
		dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, pc, DTB_BLOQUEADO, socketFD);
		break;
	}
	case QUANTUM_FALTANTE:
	{
		liberar_cpu(socketFD);
		deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);
		
		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		memcpy(&dtb->entrada_salidas, paquete->Payload + desplazamiento, INTSIZE);

		sentencias_globales_del_diego++;
		metricas_actualizar(dtb, pc);

		DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
		info_dtb->quantum_faltante = QUANTUM - (pc - dtb->PC);
		log_debug(logger, "PID %d\tQuantum Faltante = %d", dtb->gdtPID, info_dtb->quantum_faltante);
		dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, pc, DTB_BLOQUEADO, socketFD);
		break;
	}

	case DTB_EJECUTO:
	{
		liberar_cpu(socketFD);
		deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);
		
		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		metricas_actualizar(dtb, pc);
		if(verificar_si_murio(dtb, lista_ejecutando, pid, pc))
			break;

		dtb_actualizar(dtb, lista_ejecutando, lista_listos, pc, DTB_LISTO, socketFD);
		break;
	}
	case DTB_FINALIZAR: //Aca es cuando el dtb finaliza "normalmente". Lo detecta CPU
	{
		liberar_cpu(socketFD);
		deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);
	
		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		metricas_actualizar(dtb, pc);
		dtb_finalizar(dtb, lista_ejecutando, pid, pc);
		break;
	}
	case WAIT:
	{
		t_recurso *recurso = recurso_recibir(paquete->Payload, &pid, &pc, WAIT);
		log_recurso(logger, recurso, "Llego wait de recurso:");
		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		recurso_wait(recurso, dtb->gdtPID, pc, socketFD);
		verificar_si_murio(dtb, lista_ejecutando, pid, pc);
		break;
	}
	case SIGNAL:
	{
		t_recurso *recurso = recurso_recibir(paquete->Payload, &pid, &pc, SIGNAL);
		log_debug(logger, "El pid %d pide signal a %s", pid, recurso->id);
		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		metricas_actualizar(dtb, pc);
		recurso_signal(recurso, dtb->gdtPID, pc, socketFD);
		verificar_si_murio(dtb, lista_ejecutando, pid, pc);
		break;
	}
	case CLOSE:
	{
		memcpy(&pid, paquete->Payload, INTSIZE);
		desplazamiento += INTSIZE;
		ArchivoAbierto *cerrado = DTB_leer_struct_archivo(paquete->Payload, &desplazamiento);

		DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
		log_archivo(logger, cerrado, "GDT %d cerro:", dtb->gdtPID);

		void actualizar_segmentos(void *_archivo)
		{
			ArchivoAbierto *archivo = (ArchivoAbierto *)_archivo;
			if(archivo->segmento > cerrado->segmento)
				archivo->segmento--;
		}

		list_iterate(dtb->archivosAbiertos, actualizar_segmentos);
		list_remove_and_destroy_by_condition(dtb->archivosAbiertos,
						LAMBDA (bool _(void *_archivo)
						{ return coincide_archivo(_archivo, cerrado->path); }),
						liberar_archivo_abierto);			
		break;
	}
	default:
	{
		liberar_cpu(socketFD);
		manejar_errores_cpu(paquete);
		break;
	}
	}
}

void manejar_errores_cpu(Paquete *paquete)
{
	u_int32_t pid = 0, pc = 0;
	int desplazamiento = 0;
	deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);
	DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
	Tipo tipo = paquete->header.tipoMensaje;
	metricas_actualizar(dtb, pc);
	dtb_finalizar(dtb, lista_ejecutando, pid, pc);	
	
	switch(tipo)
	{
	case ABORTARA:
		log_error(logger, "Asignar Error %d: El archivo no se encuentra abierto por GDT %d", ABORTARA, dtb->gdtPID);
		break;
	case ABORTARF:
		log_error(logger, "Flush Error %d: El archivo no se encuentra abierto por GDT %d", ABORTARF, dtb->gdtPID);
		break;
	case ABORTARC:
		log_error(logger, "Cerrar Error %d: El archivo no se encuentra abierto por GDT %d", ABORTARC, dtb->gdtPID);
		break;
	case FALLO_DE_SEGMENTO_ASIGNAR:
		log_error(logger, "Asignar Error %d: Fallo de segmento en FM9 de GDT %d", FALLO_DE_SEGMENTO_ASIGNAR, pid);
		break;
	case ESPACIO_INSUFICIENTE_ASIGNAR:
		log_error(logger, "Asignar Error %d: Espacio insuficiente en FM9 de GDT %d", ESPACIO_INSUFICIENTE_ASIGNAR, pid);
		break;
	case FALLO_DE_SEGMENTO_CLOSE:
		log_error(logger, "Close Error %d: Fallo de segmento en FM9 de GDT %d", FALLO_DE_SEGMENTO_CLOSE, pid);
		break;
	}
}

void metricas_actualizar(DTB *dtb, u_int32_t pc)
{
	sem_wait(&sem_metricas);
	log_debug(logger, "pc nuevo: %d - pc viejo: %d", pc, dtb->PC);
	u_int32_t sentencias_ejecutadas = pc - dtb->PC;
	log_debug(logger, "GDT %d ejecuto %d sentencias", dtb->gdtPID, sentencias_ejecutadas);

	actualizar_sentencias_en_nuevos(sentencias_ejecutadas);
	actualizar_sentencias_en_no_finalizados(sentencias_ejecutadas);
	sentencias_totales += sentencias_ejecutadas;
	sem_post(&sem_metricas);
}

void actualizar_sentencias_en_no_finalizados(u_int32_t sentencias_ejecutadas)
{
	void _sumar_sentencias_en_no_finalizados(void *_info_dtb)
	{
		DTB_info *info_dtb = (DTB_info *)_info_dtb;
		info_dtb->sentencias_hasta_finalizar += (double) sentencias_ejecutadas;
		log_debug(logger, "GDT %d: sentencias_hasta_finalizar: %.2f", info_dtb->gdtPID, info_dtb->sentencias_hasta_finalizar);
	}

	t_list *info_no_fin = list_filter(lista_info_dtb, es_no_finalizado);
	list_iterate(info_no_fin, _sumar_sentencias_en_no_finalizados);
	list_destroy(info_no_fin);
}

void actualizar_sentencias_en_nuevos(u_int32_t sentencias_ejecutadas)
{
	void _sumar_sentencias_en_nuevo(void *_info_dtb)
	{
		DTB_info *info_dtb = (DTB_info *)_info_dtb;
		info_dtb->sentencias_en_nuevo += (double) sentencias_ejecutadas;
	}

	t_list *info_nuevos = list_filter(lista_info_dtb, es_nuevo);
	list_iterate(info_nuevos, _sumar_sentencias_en_nuevo);
	list_destroy(info_nuevos);
}

bool es_no_finalizado(void *_info_dtb)
{
	DTB_info *info_dtb = (DTB_info *)_info_dtb;
	return info_dtb->estado != DTB_FINALIZADO;
}

bool es_nuevo(void * _info_dtb)
{
	DTB_info *info_dtb = (DTB_info *)_info_dtb;
	return info_dtb->estado == DTB_NUEVO;
}

bool verificar_si_murio(DTB *dtb, t_list *lista_origen, u_int32_t pid, u_int32_t pc)
{
	DTB_info *info_dtb = info_asociada_a_pid(pid);
	
	if(info_dtb->kill)
		dtb_finalizar(dtb, lista_origen, pid, pc);

	return info_dtb->kill;
}

void seguir_ejecutando(int socket)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->header = cargar_header(0, SIGASIGA, SAFA);

	EnviarPaquete(socket, paquete);
	log_header(logger, paquete, "Envie a CPU %d que siga ejecutando", socket);
	free(paquete);
}

void avisar_desalojo_a_cpu(int socket)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->header = cargar_header(0, ROJADIRECTA, SAFA);

	EnviarPaquete(socket, paquete);
	log_header(logger, paquete, "Envie a CPU %d que desaloje", socket);
	free(paquete);
}

void recurso_wait(t_recurso *recurso, int pid, int pc, int socket)
{
	recurso->semaforo--;
	if (recurso->semaforo < 0)
	{
		log_debug(logger, "Tengo %d recursos bloqueados", list_size(recurso->pid_bloqueados));
		Pid *pid_bloquear = malloc(sizeof(Pid));
		pid_bloquear->pid = pid;
		list_add(recurso->pid_bloqueados, pid_bloquear);
		log_debug(logger, "Tengo %d recursos bloqueados", list_size(recurso->pid_bloqueados));	
		log_recurso(logger, recurso, "Hice el wait de recurso:");
		avisar_desalojo_a_cpu(socket);
		log_debug(logger, "El pid %d quedo bloqueado esperando el recurso %s", pid, recurso->id);
		return;
	}
	
	log_recurso(logger, recurso, "Hice el wait de recurso:");
	recurso_asignar_a_pid(recurso, pid);
	seguir_ejecutando(socket);
}

void recurso_signal(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
{
	recurso->semaforo++;
	log_recurso(logger, recurso, "Le hice signal al recurso:");
	if (recurso->semaforo <= 0)
		dtb_signal(recurso);

	seguir_ejecutando(socket);
}

t_recurso *recurso_recibir(void *payload, int *pid, int *pc, Tipo senial)
{
	int desplazamiento = 0;
	char *id_recurso = string_deserializar(payload, &desplazamiento);
	log_debug(logger, "Recurso recibido: %s", id_recurso);

	deserializar_pid_y_pc(payload, pid, pc, &desplazamiento);

	log_debug(logger, "Busco el recurso %s para el pid %d", id_recurso, *pid);
	t_recurso *recurso = recurso_encontrar(id_recurso);
	if (recurso == NULL)
	{
		log_debug(logger, "El recurso no existe");
		int flag = senial == WAIT;
		recurso = recurso_crear(id_recurso, flag);
	}
	free(id_recurso);

	return recurso;
}

void *config_cpu_serializar(int *tamanio_payload)
{
	void *payload = malloc(INTSIZE);
	int desplazamiento = 0;

	memcpy(payload, &QUANTUM, INTSIZE);
	desplazamiento += INTSIZE;

	int tamanio_serializado = 0;
	void *algoritmo_serializado = string_serializar(ALGORITMO_PLANIFICACION, &tamanio_serializado);
	payload = realloc(payload, desplazamiento + tamanio_serializado);

	memcpy(payload + desplazamiento, algoritmo_serializado, tamanio_serializado);
	desplazamiento += tamanio_serializado;
	free(algoritmo_serializado);

	*tamanio_payload = desplazamiento;
	return payload;
}

void enviar_handshake_cpu(int socketFD)
{
	int tamanio_payload = 0;
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->Payload = config_cpu_serializar(&tamanio_payload);
	paquete->header = cargar_header(tamanio_payload, ESHANDSHAKE, SAFA);
	EnviarPaquete(socketFD, paquete);
	log_header(logger, paquete, "Envie Handshake a CPU %d", socketFD);
	free(paquete->Payload);
	free(paquete);
}

void enviar_handshake_diego(int socketFD)
{
	Paquete paquete;
	paquete.header = cargar_header(0, ESHANDSHAKE, SAFA);
	EnviarPaquete(socketFD, &paquete);
	log_header(logger, &paquete, "Envie handshake al diego en socket %d", socketFD);
}

void event_watcher()
{
	while(true)
	{
		int multip_vieja = MULTIPROGRAMACION;
		char buffer[BUF_INOTIFY_LEN];

		int fd_config = inotify_init();
		if(fd_config < 0)
			log_error(logger, "Fallo creacion de File Descriptor para SAFA.config (inotify_init)");
		
		int watch_descriptor = inotify_add_watch(fd_config, "/home/utnso/tp-2018-2c-Nene-Malloc/safa/src/SAFA.config", IN_MODIFY);
		if(watch_descriptor < 0)
			log_error(logger, "Fallo creacion de observador de eventos en archivos (inotify_add_watch)");
		
		int length = read(fd_config, buffer, BUF_INOTIFY_LEN);
		if(length < 0)
			log_error(logger, "Fallo lectura de File Descriptor para observador de eventos (read)");
		
		int offset = 0;

		while(offset < length)
		{
			struct inotify_event *event = (struct inotify_event *)(&buffer[offset]);

			if(event->len)
			{
				if(event->mask & IN_MODIFY)
				{
					if(event->mask & IN_ISDIR)
						log_info(logger, "El directorio %s fue modificado", event->name);
					else
						log_info(logger, "El archivo %s fue modificado", event->name);

				}
			}
			offset += sizeof (struct inotify_event) + event->len;
		}
		inotify_rm_watch(fd_config, watch_descriptor);
		close(fd_config);

		usleep(1*1000);

		obtener_valores_archivo_configuracion();
		log_info(logger, "Los nuevos valores del archivo SAFA.config son: ");
		imprimir_archivo_configuracion();
		list_iterate(lista_cpu, enviar_valores_config);
		
		int dif = MULTIPROGRAMACION - multip_vieja;
		if(dif < 0)
			while(dif)
			{
				sem_wait(&sem_multiprogramacion);
				dif++;
			}
		if(dif > 0)
			while(dif--)
				sem_post(&sem_multiprogramacion);
		int a = 0;
		sem_getvalue(&sem_multiprogramacion, &a);
		log_info(logger, "sem_multiprogramacion es %d", a);
	}
}

void enviar_valores_config(void *_cpu)
{
	t_cpu *cpu = (t_cpu *)_cpu;
	int tamanio_payload = 0;
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->Payload = config_cpu_serializar(&tamanio_payload);
	paquete->header = cargar_header(tamanio_payload, CAMBIO_CONFIG, SAFA);
	EnviarPaquete(cpu->socket, paquete);
	log_header(logger, paquete, "Envie valores de config a CPU %d", cpu->socket);
	free(paquete->Payload);
	free(paquete);
}

int main(void)
{
	crear_loggers();
	obtener_valores_archivo_configuracion();
	imprimir_archivo_configuracion();
	inicializar_variables();
	// Prueba de que carga el valor correcto. Se borra despues

	pthread_create(&hilo_consola, NULL, (void *)consola, NULL);

	pthread_create(&hilo_plp, NULL, (void *)planificador_largo_plazo, NULL);

	pthread_create(&hilo_pcp, NULL, (void *)planificador_corto_plazo, NULL);

	pthread_create(&hilo_event_watcher, NULL, (void *)event_watcher, NULL);


	ServidorConcurrente(IP, PUERTO, SAFA, &lista_hilos, &end, accion);

	pthread_join(hilo_consola, NULL);
	pthread_join(hilo_pcp, NULL);
	pthread_join(hilo_plp, NULL);
	pthread_join(hilo_event_watcher, NULL);

	return EXIT_SUCCESS;
}