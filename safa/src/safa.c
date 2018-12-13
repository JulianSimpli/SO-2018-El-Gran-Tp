#include "safa.h"
#include "planificador.h"
#include <sys/inotify.h>
#include <commons/string.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

int transfer_size = 0;

/*Creación de Logger*/
void crear_loggers()
{
	logger = log_create("logs/safa.log", "safa", true, LOG_LEVEL_DEBUG);
	logger_fin = log_create ("logs/DTB_finalizados.log", "safa", true, LOG_LEVEL_INFO);
	log_info(logger, "Iniciando SAFA.log");
	log_info(logger_fin, "INFORMACION DE DTBs FINALIZADOS");
}

void obtener_valores_archivo_configuracion()
{
	t_config *arch = config_create("./SAFA.config");
	IP = string_duplicate(config_get_string_value(arch, "IP"));
	log_info(logger, "IP: %s", IP);
	PUERTO = config_get_int_value(arch, "PUERTO");
	log_info(logger, "Puerto: %d", PUERTO);
	ALGORITMO_PLANIFICACION = string_duplicate(config_get_string_value(arch, "ALGORITMO"));
	log_info(logger, "Algoritmo de planificacion: %s", ALGORITMO_PLANIFICACION);
	QUANTUM = config_get_int_value(arch, "QUANTUM");
	log_info(logger, "Quantum: %d", QUANTUM);
	MULTIPROGRAMACION = config_get_int_value(arch, "MULTIPROGRAMACION");
	log_info(logger, "Grado de multiprogramacion: %d", MULTIPROGRAMACION);
	RETARDO_PLANIF = config_get_int_value(arch, "RETARDO_PLANIF");
	log_info(logger, "Retardo de planificacion: %d", RETARDO_PLANIF);
	config_destroy(arch);
}

void imprimir_archivo_configuracion()
{
	printf("Configuración:\n"
	       "IP=%s\n"
	       "PUERTO=%d\n"
	       "ALGORITMO_PLANIFICACION=%s\n"
	       "QUANTUM=%d\n"
	       "MULTIPROGRAMACION=%d\n"
	       "RETARDO_PLANIF=%d\n",
	       IP, PUERTO, ALGORITMO_PLANIFICACION, QUANTUM, MULTIPROGRAMACION, RETARDO_PLANIF);
}

void inicializar_variables() {
	crear_listas();
	llenar_lista_estados();
	sem_init(&mutex_handshake_diego, 0, 0);
	sem_init(&mutex_handshake_cpu, 0, 0);
	sem_init(&sem_ejecutar, 0, 0);
	sem_init(&sem_multiprogramacion, 0, MULTIPROGRAMACION);
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
	log_info(logger, "Esperando que conecte el diego y al menos 1 CPU\nSAFA esta en estado corrupto\n");
	sem_wait(&mutex_handshake_diego);
	log_debug(logger, "Handshake con el diego concretado\n");
	sem_wait(&mutex_handshake_cpu);
	log_debug(logger, "Handshake con un cpu concretado\n");
	log_info(logger, "SAFA esta en estado operativo\n");
	log_info(logger, "Puede usar la consola\n");

	char *linea;
	while (true)
	{
		linea = (char *)readline(">> ");
		if (linea)
			add_history(linea);
		char **lineaSpliteada = string_split(linea, " ");
		if (lineaSpliteada[0] == NULL)
			continue;
		printf("operacion es %s\n", lineaSpliteada[0]);
		parseo_consola(lineaSpliteada[0], lineaSpliteada[1]);
		free(linea);
		if (!lineaSpliteada[0])
			free(lineaSpliteada[0]);
		if (!lineaSpliteada[0])
			free(lineaSpliteada[1]);
		free(lineaSpliteada);
	}
}

void parseo_consola(char *operacion, char *primer_parametro)
{
	u_int32_t pid = 0;
	if (string_equals_ignore_case(operacion, EJECUTAR))
	{
		if (primer_parametro == NULL)
			printf("expected ejecutar <path>\n");
		else
		{
			printf("path a ejecutar es %s\n", primer_parametro);
			ejecutar(primer_parametro);
		}
	}
	else if (string_equals_ignore_case(operacion, STATUS))
	{
		if (primer_parametro != NULL)
		{
			pid = atoi(primer_parametro);
			printf("Mostrar status de proceso con PID %i\n", pid);
			gdt_status(pid);
		}
		else
		{
			printf("status no trajo parametros. Se mostraran estados de las colas\n");
			status();
		}
	}
	else if (string_equals_ignore_case(operacion, FINALIZAR))
	{
		if (primer_parametro == NULL)
		{
			printf("expected finalizar <pid>\n");
			return;
		}

		pid = atoi(primer_parametro);
		printf("pid a finalizar es %i\n", pid);
		finalizar(pid);
	}
	else if (string_equals_ignore_case(operacion, METRICAS))
	{
		if (primer_parametro != NULL)
		{
			pid = atoi(primer_parametro);
			printf("pid a mostrar metricas es %i\n", pid);
			gdt_metricas(pid);
		}
		else
		{
			printf("metricas no trajo parametros. Solo se muestran estadisticas del sistema\n");
			metricas();
		}
	}
	else if (string_equals_ignore_case(operacion, LIBERAR))
	{
		if (primer_parametro != NULL)
		{
			int procesos_a_liberar = atoi(primer_parametro);
			printf("Se van a liberar %d procesos\n", procesos_a_liberar);
			liberar_parte_de_memoria(procesos_a_liberar);
		}
		else
		{
			printf("Se van a liberar todos los procesos de finalizados (%d)\n", list_size(lista_finalizados));
			liberar_memoria();
		}
	}
	else
		printf("No se conoce la operacion\n");
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
		switch (paquete.header.emisor)
		{
		case ELDIEGO:
		{
			if (paquete.header.tipoMensaje != ESHANDSHAKE && paquete.header.tamPayload > 0) {
				paquete.Payload = malloc(paquete.header.tamPayload);
				recibir_partes(socketFD, paquete.Payload, paquete.header.tamPayload);
			}

			manejar_paquetes_diego(&paquete, socketFD);
			break;
		}
		case CPU:
		{
			if (paquete.header.tamPayload > 0) {
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
	}
	// Si sale del while hubo error o desconexion
	manejar_desconexion(socketFD);
	if (paquete.Payload != NULL)
		free(paquete.Payload);

	close(socketFD);
}

void manejar_desconexion(int socket)
{
	if(socket == socket_diego)
	{
		log_error(logger, "Desconexion en el socket %d, donde estaba El Diego", socket_diego);
		printf("Se desconecto El Diego, que hacemo?\n");
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
	printf("Se desconecto la cpu %d\n", socket);

	bool _dtb_compara_socket(void *_dtb)
	{
		return dtb_coincide_socket(socket, _dtb);
	}

	DTB *dtb = list_find(lista_ejecutando, _dtb_compara_socket);
	if(dtb != NULL)
	{
		dtb_actualizar(dtb, lista_ejecutando, lista_listos, dtb->PC, DTB_LISTO, socket);
		log_info(logger, "GDT %d desalojado de cpu %d desconectada", dtb->gdtPID, socket);
		printf("Se desalojo al GDT %d de la cpu %d desconectada\n", dtb->gdtPID, socket);
	}

	bool _compara_cpu(void *_cpu)
	{
		return cpu_coincide_socket(socket, _cpu);
	}
	list_remove_and_destroy_by_condition(lista_cpu, _compara_cpu, free);
	log_info(logger, "Cpu %d removida del sistema", socket);
	printf("Cpu %d removida del sistema\n", socket);

	if(list_is_empty(lista_cpu))
	{
		log_info(logger, "Se desconecto la ultima cpu del sistema. Esperando 30 segundos por la conexion de otras.");
		printf("Se desconecto la ultima cpu del sistema.\n"
				"Si no se conecta otra en 30 segundos, el sistema finalizara\n");
		sleep(30);
		if(list_is_empty(lista_cpu))
		{
			log_info(logger, "Sistema terminado correctamente tras la desconexion de todas las CPUs");
			printf("Terminando SAFA.\n"
					"Hasta la proxima!\n");
			exit(1);
		}	

		// Esperar a que conecte otra por x tiempo?
		// sleep(30) Si sigue siendo 0 el list size exit(1)
		// Si entro otra cpu, continua ejecutando
	}
}

void manejar_paquetes_diego(Paquete *paquete, int socketFD)
{
	log_debug(logger, "Interpreto mensajes del diego");

	u_int32_t pid = 0;
	int tam_pid = INTSIZE;
	switch (paquete->header.tipoMensaje)
	{
		case ESHANDSHAKE:
		{
			log_debug(logger, "ESHANDSHAKE");
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
			log_debug(logger, "DUMMY_SUCCESS");
			memcpy(&pid, paquete->Payload, tam_pid);
			DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
			log_info(logger, "GDT %d fue cargado en memoria correctamente", dtb->gdtPID);

			ArchivoAbierto *escriptorio_cargado = DTB_leer_struct_archivo(paquete->Payload, &tam_pid);
			ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
			escriptorio->cantLineas = escriptorio_cargado->cantLineas;
			escriptorio->segmento = escriptorio_cargado->segmento;
			escriptorio->pagina = escriptorio_cargado->pagina;

			liberar_archivo_abierto(escriptorio_cargado);

			if(verificar_si_murio(dtb, lista_nuevos, pid, dtb->PC))
				break;
			
			pasaje_a_ready(dtb->gdtPID);
			break;
		}
		case DUMMY_FAIL_CARGA:
		{
			log_debug(logger, "DUMMY_FAIL_CARGA");
			memcpy(&pid, paquete->Payload, tam_pid);
			DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
			log_error(logger, "Fallo la carga en memoria del GDT %d", dtb->gdtPID);
			bloquear_dummy(lista_ejecutando, dtb->gdtPID);
			dtb_actualizar(dtb, lista_nuevos, lista_finalizados, dtb->PC, DTB_FINALIZADO, 0);
			break;
		}

		case DUMMY_FAIL_NO_EXISTE:
		{
			log_debug(logger, "DUMMY_FAIL_NO_EXISTE");
			memcpy(&pid, paquete->Payload, tam_pid);
			DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
			ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
			log_error(logger, "No existe el escriptorio %s del GDT %d", escriptorio->path, dtb->gdtPID);
			bloquear_dummy(lista_ejecutando, dtb->gdtPID);
			dtb_actualizar(dtb, lista_nuevos, lista_finalizados, dtb->PC, DTB_FINALIZADO, 0);
			break;
		}

		case DTB_SUCCESS: //Incluye flush, crear, borrar.
		{
			memcpy(&pid, paquete->Payload, tam_pid);

			DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
			log_info(logger, "GDT %d realizo la operacion bloqueante correctamente");

			info_dtb->tiempo_respuesta = medir_tiempo(0, (info_dtb->tiempo_ini), (info_dtb->tiempo_fin));

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
			log_debug(logger, "ARCHIVO_ABIERTO");
			memcpy(&pid, paquete->Payload, tam_pid);
			DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);

			ArchivoAbierto *archivo = DTB_leer_struct_archivo(paquete->Payload, &tam_pid);
			list_add(dtb->archivosAbiertos, archivo);
			log_info(logger, "GDT %d abrio %s", dtb->gdtPID, archivo->path);
			info_dtb->tiempo_respuesta = medir_tiempo(0, (info_dtb->tiempo_ini), (info_dtb->tiempo_fin));

			if(verificar_si_murio(dtb, lista_bloqueados, pid, dtb->PC))
				break;
			
			dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC, DTB_LISTO, info_dtb->socket_cpu);
			break;
		}

		case PATH_INEXISTENTE:
		{
			memcpy(&pid, paquete->Payload, INTSIZE);
			DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
			log_error(logger, "Abrir Error %d: GDT %d intento abrir un path inexistente", PATH_INEXISTENTE, pid);
			dtb_finalizar(dtb, lista_bloqueados, pid, dtb->PC);
			break;
		}

		case ESPACIO_INSUFICIENTE_ABRIR:
		{
			memcpy(&pid, paquete->Payload, INTSIZE);
			DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);	
			log_error(logger, "Abrir Error %d: GDT %d no pudo abrir archivo por espacio insuficiente en FM9", ESPACIO_INSUFICIENTE_ABRIR, pid);
			dtb_finalizar(dtb, lista_bloqueados, pid, dtb->PC);
		}

		case DTB_FINALIZAR:
		{
			sem_post(&sem_multiprogramacion);
			memcpy(&pid, paquete->Payload, INTSIZE);
			DTB_info *info_dtb;
			t_list *lista_actual;
			DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);
			log_info(logger, "GDT %d removido de memoria", dtb->gdtPID);
			dtb_actualizar(dtb, lista_actual, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
		}
	}
	if(paquete->header.tamPayload > 0)
		free(paquete->Payload);
}

void manejar_paquetes_CPU(Paquete *paquete, int socketFD)
{
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
		list_add(lista_cpu, cpu_nuevo);
		log_info(logger, "llegada cpu con id %i", cpu_nuevo->socket);
		sem_post(&mutex_handshake_cpu);
		enviar_handshake_cpu(socketFD);
		break;
	}

	case DUMMY_BLOQUEA:
		liberar_cpu(socketFD);
		log_debug(logger, "DUMMY_BLOQUEA");
		memcpy(&pid, paquete->Payload, INTSIZE);
		log_info(logger, "Se ejecuto el dummy de GDT %d", pid);
		bloquear_dummy(lista_ejecutando, pid);
		break;

        case DTB_BLOQUEAR:
		{
			log_debug(logger, "DTB_BLOQUEAR");
			liberar_cpu(socketFD);
			deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);

			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			memcpy(&dtb->entrada_salidas, paquete->Payload + desplazamiento, INTSIZE);
			actualizar_sentencias_al_diego(dtb);
			sentencias_globales_del_diego++;
			metricas_actualizar(dtb, pc);
			DTB_info* info_dtb = info_asociada_a_pid(dtb->gdtPID);			
			dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, pc, DTB_BLOQUEADO, socketFD);
			//medir_tiempo(1,(info_dtb->tiempo_ini), (info_dtb->tiempo_fin)); Arreglar
			break;
    	}

        case PROCESS_TIMEOUT:
		{
			liberar_cpu(socketFD);
			deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);
			
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			metricas_actualizar(dtb, pc);
			dtb_actualizar(dtb, lista_ejecutando, lista_listos, pc, DTB_LISTO, socketFD);
			break;
        }

		case QUANTUM_FALTANTE:
		{
			liberar_cpu(socketFD);
			deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);
			
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			metricas_actualizar(dtb, pc);

			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
			info_dtb->quantum_faltante = QUANTUM - (pc - dtb->PC);
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
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			metricas_actualizar(dtb, pc);
			if(verificar_si_murio(dtb, lista_ejecutando, pid, pc))
			{
				avisar_desalojo_a_cpu(socketFD);
				break;
			}
			recurso_wait(recurso, dtb->gdtPID, pc, socketFD);
			break;
		}
		case SIGNAL:
		{
			t_recurso *recurso = recurso_recibir(paquete->Payload, &pid, &pc, SIGNAL);
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			metricas_actualizar(dtb, pc);
			if(verificar_si_murio(dtb, lista_ejecutando, pid, pc))
			{
				avisar_desalojo_a_cpu(socketFD);
				break;
			}
			recurso_signal(recurso, dtb->gdtPID, pc, socketFD);
			break;
		}
		case ABORTAR:
		{
			liberar_cpu(socketFD);
			deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);

			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			log_error(logger, "Asignar Error %d: El archivo no se encuentra abierto por GDT %d", ABORTAR, dtb->gdtPID);
			metricas_actualizar(dtb, pc);
			
			dtb_finalizar(dtb, lista_ejecutando, pid, pc);
			break;
		}
		case ABORTARF:
		{
			liberar_cpu(socketFD);
			deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);

			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			log_error(logger, "Flush Error %d: El archivo no se encuentra abierto por GDT %d", ABORTARF, dtb->gdtPID);
			metricas_actualizar(dtb, pc);
			
			dtb_finalizar(dtb, lista_ejecutando, pid, pc);
			break;
		}
		case ABORTARC:
		{
			liberar_cpu(socketFD);
			deserializar_pid_y_pc(paquete->Payload, &pid, &pc, &desplazamiento);

			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			log_error(logger, "Cerrar Error %d: El archivo no se encuentra abierto por GDT %d", ABORTARC, dtb->gdtPID);
			metricas_actualizar(dtb, pc);
			
			dtb_finalizar(dtb, lista_ejecutando, pid, pc);
			break;
		}
		case FALLO_DE_SEGMENTO_ASIGNAR:
		{
			liberar_cpu(socketFD);

			memcpy(&pid, paquete->Payload, INTSIZE);
			desplazamiento += INTSIZE;
			memcpy(&pc, paquete->Payload + desplazamiento, INTSIZE);
			desplazamiento += INTSIZE;
			
			log_error(logger, "Asignar Error %d: Falla de segmento en FM9 de GDT %d al intentar hacer asignacion", ESPACIO_INSUFICIENTE_ASIGNAR, pid);
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			dtb_finalizar(dtb, lista_ejecutando, pid, pc);
			break;
		}
		case ESPACIO_INSUFICIENTE_ASIGNAR:
		{
			liberar_cpu(socketFD);

			memcpy(&pid, paquete->Payload, INTSIZE);
			desplazamiento += INTSIZE;
			memcpy(&pc, paquete->Payload + desplazamiento, INTSIZE);
			desplazamiento += INTSIZE;

			log_error(logger, "Asignar Error %d: Espacio insuficiente en FM9 de GDT %d al intentar hacer asignacion", ESPACIO_INSUFICIENTE_ASIGNAR, pid);
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);			
			dtb_finalizar(dtb, lista_ejecutando, pid, pc);
			break;
		}
		case FALLO_DE_SEGMENTO_CLOSE:
		{
			liberar_cpu(socketFD);

			memcpy(&pid, paquete->Payload, INTSIZE);
			desplazamiento += INTSIZE;
			memcpy(&pc, paquete->Payload + desplazamiento, INTSIZE);
			desplazamiento += INTSIZE;

			log_error(logger, "Close Error %d: Fallo de segmento en FM9 de GDT %d al intentar hacer close", FALLO_DE_SEGMENTO_CLOSE, pid);
			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);			
			dtb_finalizar(dtb, lista_ejecutando, pid, pc);
			break;
		}
		case CLOSE:
		{
			log_debug(logger, "CLOSE");
			memcpy(&pid, paquete->Payload, INTSIZE);
			desplazamiento += INTSIZE;
			ArchivoAbierto *cerrado = DTB_leer_struct_archivo(paquete->Payload, &desplazamiento);

			DTB *dtb = dtb_encuentra(lista_ejecutando, pid, GDT);
			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
			log_info(logger, "GDT %d cerro %s", dtb->gdtPID, cerrado->path);

			list_remove_and_destroy_by_condition(dtb->archivosAbiertos, LAMBDA (bool _(void *_archivo) {return coincide_archivo(_archivo, cerrado->path); }), liberar_archivo_abierto);			
			break;
		}
	}
}

void metricas_actualizar(DTB *dtb, u_int32_t pc)
{
	u_int32_t sentencias_ejecutadas = pc - dtb->PC;

	actualizar_sentencias_en_nuevos(sentencias_ejecutadas);
	actualizar_sentencias_en_no_finalizados(sentencias_ejecutadas);
	sentencias_totales += sentencias_ejecutadas;
}

void actualizar_sentencias_al_diego(DTB *dtb)
{
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	info_dtb->sentencias_al_diego++;
}

void actualizar_sentencias_en_no_finalizados(u_int32_t sentencias_ejecutadas)
{
	void _sumar_sentencias_en_no_finalizados(void *_info_dtb)
	{
		DTB_info *info_dtb = (DTB_info *)_info_dtb;
		info_dtb->sentencias_hasta_finalizar += sentencias_ejecutadas;
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
		info_dtb->sentencias_en_nuevo += sentencias_ejecutadas;
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
	free(paquete);
}

void avisar_desalojo_a_cpu(int socket)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->header = cargar_header(0, ROJADIRECTA, SAFA);

	EnviarPaquete(socket, paquete);
	free(paquete);
}

void recurso_wait(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
{
	recurso->semaforo--;
	if (recurso->semaforo < 0)
	{
		list_add(recurso->pid_bloqueados, &pid);
		avisar_desalojo_a_cpu(socket);
		log_debug(logger, "El recurso queda bloqueado");
		return;
	}
	
	recurso_asignar_a_pid(recurso, pid);
	seguir_ejecutando(socket);
}

void recurso_signal(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
{
	recurso->semaforo++;
	if (recurso->semaforo <= 0)
	{
		dtb_signal(recurso);
		seguir_ejecutando(socket);
	}
	else
		seguir_ejecutando(socket);
}

t_recurso *recurso_recibir(void *payload, int *pid, int *pc, Tipo senial)
{
	int desplazamiento = 0;
	char *id_recurso = string_deserializar(payload, &desplazamiento);

	deserializar_pid_y_pc(payload, pid, pc, &desplazamiento);

	t_recurso *recurso = recurso_encontrar(id_recurso);
	if (recurso == NULL)
	{
		int flag = senial != SIGNAL;
		recurso = recurso_crear(id_recurso, flag);
	}

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
	free(paquete->Payload);
	free(paquete);
}

void enviar_handshake_diego(int socketFD)
{
	Paquete paquete;
	paquete.header = cargar_header(0, ESHANDSHAKE, SAFA);
	log_info(logger, "Handshake a diego: %d", socketFD);
	EnviarPaquete(socketFD, &paquete);
}

void event_watcher()
{
	while(true)
	{
		//sleep(1);
		int multip_vieja = MULTIPROGRAMACION;
		char buffer[BUF_INOTIFY_LEN];

		int fd_config = inotify_init();
		if(fd_config < 0)
			log_error(logger, "Fallo creacion de File Descriptor para SAFA.config (inotify_init)");
		
		int watch_descriptor = inotify_add_watch(fd_config, "/home/utnso/tp-2018-2c-Nene-Malloc/safa/src", IN_MODIFY);
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

		sleep(1);

		obtener_valores_archivo_configuracion();
		printf("Los nuevos valores del archivo SAFA.cfg son: \n");
		imprimir_archivo_configuracion();
		list_iterate(lista_cpu, enviar_valores_config);
		
		int dif = MULTIPROGRAMACION - multip_vieja;
		printf("dif es %d\n", dif);
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
		printf("sem_multiprogramacion es %d\n", a);
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
	int a = 0;
	sem_getvalue(&sem_multiprogramacion, &a);
	printf("sem_multiprogramacion es %d\n", a);
	
	// Esto creo lo usa @nicoPosty para 1 metrica
	clock_t t = clock();
	clock_t t2 = clock();
	char* estado_1 = "EJECUTANDO";

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
