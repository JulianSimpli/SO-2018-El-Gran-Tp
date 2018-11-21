#include "safa.h"

/*Creación de Logger*/
void crear_logger()
{
	logger = log_create("safa.log", "safa", true, LOG_LEVEL_INFO);
}


void crear_logger_finalizados (){
	logger_fin = log_create ("Archivos Finalizados.log", "safa", true, LOG_LEVEL_INFO);
}


void inicializar_variables() {
	crear_listas();
	llenar_lista_estados();
	sem_init(&mutex_handshake_diego, 0, 0);
	sem_init(&mutex_handshake_cpu, 0, 0);
}

void crear_listas()
{
	lista_cpu = list_create();
	lista_nuevos = list_create();
	lista_listos = list_create();
	lista_ejecutando = list_create();
	lista_bloqueados = list_create();
	lista_finalizados = list_create();
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

void obtener_valores_archivo_configuracion()
{
	t_config *arch = config_create("/home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/src/SAFA.config");
	IP = "127.0.0.1";
	PUERTO = config_get_int_value(arch, "PUERTO");
	ALGORITMO_PLANIFICACION = string_duplicate(config_get_string_value(arch, "ALGORITMO"));
	QUANTUM = config_get_int_value(arch, "QUANTUM");
	MULTIPROGRAMACION = config_get_int_value(arch, "MULTIPROGRAMACION");
	RETARDO_PLANIF = config_get_int_value(arch, "RETARDO_PLANIF");
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

void consola()
{
	printf("Esperando que conecte el diego y al menos 1 CPU\nSAFA esta en estado corrupto\n");
	log_info(logger, "Esperando que conecte el diego y al menos 1 CPU. SAFA esta en estado corrupto\n");
	// sem_wait(&mutex_handshake_diego);
	// sem_wait(&mutex_handshake_cpu);
	printf("Se conectaron Diego y 1 CPU\nSAFA esta en estado operativo\nYa puede usar la consola\n");
	log_info(logger, "Se conectaron Diego y 1 CPU. SAFA esta en estado operativo");

	char *linea;
	while (true)
	{
		linea = readline(">> ");
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
	}
}

void parseo_consola(char *operacion, char *primerParametro)
{
	u_int32_t pid = 0;
	if (string_equals_ignore_case(operacion, EJECUTAR))
	{
		if (primerParametro == NULL)
		{
			printf("expected ejecutar <path>\n");
		}
		printf("path a ejecutar es %s\n", primerParametro);
		ejecutar(primerParametro);
	}
	else if (string_equals_ignore_case(operacion, STATUS))
	{
		if (primerParametro != NULL)
		{
			pid = atoi(primerParametro);
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
		if (primerParametro == NULL)
		{
			printf("expected finalizar <pid>\n");
			return;
		}

		pid = atoi(primerParametro);
		printf("pid a finalizar es %i\n", pid);
		finalizar(pid);
	}
	else if (string_equals_ignore_case(operacion, METRICAS))
	{
		if (primerParametro != NULL)
		{
			pid = atoi(primerParametro);
			printf("pid a mostrar metricas es %i\n", pid);
			//mostrarMetricasDe(pid);
		}
		else
			printf("metricas no trajo parametros. Solo se muestran estadisticas del sistema\n");
		//mostrarMetricasDelSistema();
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
	while (RecibirPaqueteServidorSafa(socketFD, SAFA, &paquete) > 0)
	{
		log_info(logger, "Emisor: %i", paquete.header.emisor);
		switch (paquete.header.emisor)
		{
		case ELDIEGO:
		{
			manejar_paquetes_diego(&paquete, socketFD);
			break;
		}
		case CPU:
		{
			manejar_paquetes_CPU(&paquete, socketFD);
			break;
		}
		}
	}
	// Caso desconexion cpu
	if (paquete.Payload != NULL)
		free(paquete.Payload);
	close(socketFD);
}

void manejar_paquetes_diego(Paquete *paquete, int socketFD)
{
	switch (paquete->header.tipoMensaje)
	{
		case ESHANDSHAKE:
		{
			sem_post(&mutex_handshake_diego);
			socket_diego = socketFD;
			log_info(logger, "llegada de el diego en socket %d", socketFD);
			enviar_handshake_diego(socketFD);
			break;
		}
		case DUMMY_SUCCES:
		{
			// Mensaje contiene pid, posicion en memoria, largo path, cantidad lineas
			u_int32_t pid;
			memcpy(&pid, paquete->Payload, sizeof(u_int32_t));
			log_info(logger, "Proceso con PID %d fue cargado en memoria", pid);
			DTB *dtb = dtb_encuentra(lista_nuevos, pid, GDT);
			DTB_info *info_dtb = info_asociada_a_pid(pid);
			if(verificar_si_murio(dtb, lista_nuevos))
				break;
			pasaje_a_ready(&pid);
			liberar_cpu(socketFD);
			log_info(logger, "Cpu %d liberada", socketFD);
			break;
		}
		case DUMMY_FAIL:
		{
			u_int32_t pid;
			DTB_info *info_dtb;
			memcpy(&pid, paquete->Payload, paquete->header.tamPayload);
			bloquear_dummy(lista_ejecutando, pid);
			log_error(logger, "Fallo la carga en memoria del %d", pid);
			list_iterate(info_dtb->recursos, forzar_signal);
			enviar_finalizar_dam(pid);
			break;
		}
//		case DTB_SUCCES:
//		{	METRICAS CLOCK
//			break;
//			//Me manda pid, archivos abiertos, datos de memoria virtual para buscar el archivo.
//		}
		case DTB_SUCCES: //DTB_DESBLOQUEAR?
		{
			u_int32_t pid;
			memcpy(&pid, paquete->Payload, paquete->header.tamPayload);

			// DTB_info *info_dtb;
			// t_list *lista_actual;
			// DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);
			// Hay que usar dtb_actualizar. Lo puede buscar en bloqueados directamente.
			// En algun caso no estaria en bloqueados?

			DTB *dtb = dtb_encuentra(lista_bloqueados, pid, GDT);
			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
			if(verificar_si_murio(dtb, lista_bloqueados))
				break;
			dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC, DTB_BLOQUEADO, info_dtb->socket_cpu);
			info_dtb->tiempo_respuesta = medir_tiempo(0, (info_dtb->tiempo_ini), (info_dtb->tiempo_fin));
			break;
		}
		case DTB_FAIL:
		{
			u_int32_t pid;
			DTB_info *info_dtb;
			memcpy(&pid, paquete->Payload, paquete->header.tamPayload);
			bloquear_dummy(lista_ejecutando, pid);
			log_error(logger, "Fallo la carga en memoria del %d", pid);
			list_iterate(info_dtb->recursos, forzar_signal);
			enviar_finalizar_dam(pid);
			break;
		}
		case DTB_FINALIZAR:
		{
			u_int32_t pid;
			memcpy(&pid, paquete->Payload, paquete->header.tamPayload);
			DTB_info *info_dtb;
			t_list *lista_actual;
			DTB *dtb = dtb_buscar_en_todos_lados(pid, &info_dtb, &lista_actual);
			if(esta_en_memoria(info_dtb)) procesos_en_memoria--;
			dtb_finalizar_desde(dtb, lista_actual);
			free(dtb);
		}
	}
}

void manejar_paquetes_CPU(Paquete *paquete, int socketFD)
{
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

        case DTB_BLOQUEAR:
		{
			// Habria que ver si algoritmo es rr o vrr para recibir quantum que queda

			liberar_cpu(socketFD);
			// METRICAS CLOCK;
			DTB *dtb = DTB_deserializar(paquete->Payload);
			dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, dtb->PC, DTB_BLOQUEADO, socketFD);
			DTB_info* info_dtb = info_asociada_a_pid(dtb->gdtPID);
			//dtb_info->tiempo_ini=clock(); LLAMAR A LA FUNCION QUE HAGA ESO
			medir_tiempo(1,(info_dtb->tiempo_ini), (info_dtb->tiempo_fin));
			break;
    	}

        case PROCESS_TIMEOUT:
		{
			liberar_cpu(socketFD);
            DTB* dtb = DTB_deserializar(paquete->Payload);
			dtb_actualizar(dtb, lista_ejecutando, lista_listos, dtb->PC, DTB_LISTO, socketFD);
			break;
        }
        case DTB_EJECUTO: // No veo diferencias con Process_timeout
		{
            DTB* dtb = DTB_deserializar(paquete->Payload);
			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
			if(verificar_si_murio(dtb, lista_ejecutando))
				break;

			liberar_cpu(socketFD);
			dtb_actualizar(dtb, lista_ejecutando, lista_listos, dtb->PC, DTB_LISTO, socketFD);
			break;
        }
		case DTB_FINALIZAR: //Aca es cuando el dtb finaliza "normalmente"
		{
			liberar_cpu(socketFD);
            DTB* dtb = DTB_deserializar(paquete->Payload);
			DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
			dtb_actualizar(dtb, lista_ejecutando, lista_finalizados, dtb->PC, DTB_FINALIZADO, socketFD);
	        loggear_finalizacion(dtb, info_dtb);
			list_iterate(info_dtb->recursos, forzar_signal);
			enviar_finalizar_dam(dtb->gdtPID);
			break;
		}
		case WAIT:
		{
			u_int32_t pid = 0;
			u_int32_t pc = 0;
			t_recurso *recurso = recurso_recibir(paquete->Payload, &pid, &pc);
			//dtb asociado a pid, y a wait le paso como parametro dtb->gdtPID porque tiene que ser dato persistido en memoria
			// si no, se va a borrar cuando salga del bloque. Lo mismo en signal.
			recurso_wait(recurso, pid, pc, socketFD);
			break;
		}
		case SIGNAL:
		{
			u_int32_t pid = 0;
			u_int32_t pc = 0;
			t_recurso *recurso = recurso_recibir(paquete->Payload, &pid, &pc);
			recurso_signal(recurso, pid, pc, socketFD);
			break;
		}
	}
}

bool verificar_si_murio(DTB *dtb, t_list *lista_origen)
{
	DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
	
	if(info_dtb->kill)
	{
		loggear_finalizacion(dtb, info_dtb);
		list_iterate(info_dtb->recursos, forzar_signal);
		dtb_actualizar(dtb, lista_origen, lista_finalizados, dtb->PC, DTB_FINALIZADO, info_dtb->socket_cpu);
		enviar_finalizar_dam(dtb->gdtPID);
	}

	return info_dtb->kill;
}

void *serializar_pid_y_pc(u_int32_t pid, u_int32_t pc, int *tam_pid_y_pc)
{
	void *payload = malloc(sizeof(u_int32_t) * 2);

	memcpy(payload + *tam_pid_y_pc, &pid, sizeof(u_int32_t));
	*tam_pid_y_pc += sizeof(u_int32_t);
	memcpy(payload + *tam_pid_y_pc, &pc, sizeof(u_int32_t));
	*tam_pid_y_pc += sizeof(u_int32_t);

	return payload;
}

void seguir_ejecutando(u_int32_t pid, u_int32_t pc, int socket)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	int tamanio_payload = 0;
	paquete->Payload = serializar_pid_y_pc(pid, pc, &tamanio_payload);
	paquete->header = cargar_header(tamanio_payload, SIGASIGA, SAFA);

	EnviarPaquete(socket, paquete);
	free(paquete->Payload);
	free(paquete);
}

DTB *dtb_bloquear(u_int32_t pid, u_int32_t pc, int socket)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	int tamanio_payload = 0;
	paquete->Payload = serializar_pid_y_pc(pid, pc, &tamanio_payload);
	paquete->header = cargar_header(tamanio_payload, ROJADIRECTA, SAFA);

	EnviarPaquete(socket, paquete);
	free(paquete);
	free(paquete->Payload);

	DTB *dtb = dtb_encuentra(lista_ejecutando, pid, 1);
	dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, pc, DTB_BLOQUEADO, socket);

	return dtb;
}

void recurso_wait(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
{
	recurso->semaforo--;
	if (recurso->semaforo < 0)
	{
		list_add(recurso->pid_bloqueados, &pid);
		dtb_bloquear(pid, pc, socket);
	}
	else
	{
		recurso_asignar_a_pid(recurso, pid);
		seguir_ejecutando(pid, pc, socket);
	}
}

void recurso_signal(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
{
	recurso->semaforo++;
	if (recurso->semaforo <= 0)
	{
		dtb_signal(recurso);
		seguir_ejecutando(pid, pc, socket);
	}
	else
		seguir_ejecutando(pid, pc, socket);
}

t_recurso *recurso_recibir(void *payload, int *pid, int *pc)
{
	int desplazamiento = 0;
	char *id_recurso = string_deserializar(payload, &desplazamiento);

	memcpy(pid, payload + desplazamiento, sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);
	memcpy(pc, payload + desplazamiento, sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);

	t_recurso *recurso = recurso_encontrar(id_recurso);
	if (recurso == NULL)
		recurso = recurso_crear(id_recurso, 1);

	return recurso;
}

void *string_serializar(char *string, int *desplazamiento)
{
	u_int32_t len_string = strlen(string);
	void *serializado = malloc(sizeof(len_string) + len_string);

	memcpy(serializado + *desplazamiento, &len_string, sizeof(len_string));
	*desplazamiento += sizeof(len_string);
	memcpy(serializado + *desplazamiento, string, len_string);
	*desplazamiento += len_string;

	return serializado;
}

char *string_deserializar(void *data, int *desplazamiento)
{
	u_int32_t len_string = 0;
	memcpy(&len_string, data + *desplazamiento, sizeof(len_string));
	*desplazamiento += sizeof(len_string);

	char *string = malloc(len_string + 1);
	memcpy(string, data + *desplazamiento, len_string);
	*(string + len_string) = '\0';
	*desplazamiento = *desplazamiento + len_string;

	return string;
}

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

bool coincide_id(void *recurso, char *id)
{
	return !strcmp(((t_recurso *)recurso)->id, id);
}

t_recurso *recurso_encontrar(char *id_recurso)
{
	bool comparar_id(void *recurso)
	{
		return coincide_id(recurso, id_recurso);
	}

	return list_find(lista_recursos_global, comparar_id);
}

void *handshake_cpu_serializar(int *tamanio_payload)
{
	void *payload = malloc(sizeof(u_int32_t));
	int desplazamiento = 0;

	memcpy(payload, &QUANTUM, sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);

	int tamanio_serializado = 0;
	void *algoritmo_serializado = string_serializar(ALGORITMO_PLANIFICACION, &tamanio_serializado);
	payload = realloc(payload, desplazamiento + tamanio_serializado);

	memcpy(payload + desplazamiento, algoritmo_serializado, tamanio_serializado);
	desplazamiento += tamanio_serializado;
	free(algoritmo_serializado); // Esto es correcto?

	*tamanio_payload = desplazamiento;
	return payload;
}

void enviar_handshake_cpu(int socketFD)
{
	int tamanio_payload = 0;
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->Payload = handshake_cpu_serializar(&tamanio_payload);
	paquete->header = cargar_header(tamanio_payload, ESHANDSHAKE, SAFA);
	EnviarPaquete(socketFD, paquete);
	free(paquete);
}

void enviar_handshake_diego(int socketFD)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	paquete->header = cargar_header(0, ESHANDSHAKE, SAFA);
	log_info(logger, "Handshake a diego: %d", socketFD );
	EnviarPaquete(socketFD, paquete);
	free(paquete);
}

int main(void)
{
	crear_logger();
	crear_logger_finalizados();
	inicializar_variables();
	obtener_valores_archivo_configuracion();
	imprimir_archivo_configuracion();
	log_info(logger, "Iniciando SAFA.log");
	log_info(logger_fin, "***INFORMACION DE ARCHIVOS FINALIZADOS***");
	clock_t t = clock();
	clock_t t2 = clock();
	char* estado_1 = "EJECUTANDO";

	pthread_create(&hilo_consola, NULL, (void *)consola, NULL);

	pthread_create(&hilo_plp, NULL, (void *)planificador_largo_plazo, NULL);

	pthread_create(&hilo_pcp, NULL, (void *)planificador_corto_plazo, NULL);

	ServidorConcurrente(IP, PUERTO, SAFA, &lista_hilos, &end, accion);

	pthread_join(hilo_consola, NULL);
	pthread_join(hilo_pcp, NULL);
	pthread_join(hilo_plp, NULL);

	return EXIT_SUCCESS;
}
