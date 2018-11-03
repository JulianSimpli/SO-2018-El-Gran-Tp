#include "safa.h"

/*Creación de Logger*/
void crearLogger() {
	logger = log_create("SAFA.log", "safa", true, LOG_LEVEL_INFO);
}

void inicializarVariables() {

	lista_cpu = list_create();
    lista_nuevos = list_create();
    lista_listos = list_create();
    lista_ejecutando = list_create();
    lista_bloqueados = list_create();
    lista_finalizados = list_create();
	stateArray[0] = lista_nuevos;
	stateArray[1] = lista_listos;
	stateArray[2] = lista_ejecutando;
	stateArray[3] = lista_bloqueados;
	stateArray[4] = lista_finalizados;
    lista_PLP = list_create();
    pthread_mutex_init(&mutex_handshake_diego, NULL);
    pthread_mutex_init(&mutex_handshake_cpu, NULL);
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("/home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/src/SAFA.config");
	IP = "127.0.0.1"; //agregado
	PUERTO = config_get_int_value(arch, "PUERTO");
	ALGORITMO_PLANIFICACION = string_duplicate(config_get_string_value(arch, "ALGORITMO"));
	QUANTUM = config_get_int_value(arch, "QUANTUM");
	MULTIPROGRAMACION = config_get_int_value(arch, "MULTIPROGRAMACION");
	RETARDO_PLANIF = config_get_int_value(arch, "RETARDO_PLANIF");
	config_destroy(arch);
}

void imprimirArchivoConfiguracion() {
	printf("Configuración:\n"
			"IP=%s\n" //agregado
			"PUERTO=%d\n"
			"ALGORITMO_PLANIFICACION=%s\n"
			"QUANTUM=%d\n"
			"MULTIPROGRAMACION=%d\n"
			"RETARDO_PLANIF=%d\n",
			IP, PUERTO, ALGORITMO_PLANIFICACION, QUANTUM, MULTIPROGRAMACION, RETARDO_PLANIF);
}
void consola() {
	char * linea;
	while (true) {
		linea = readline(">> ");
		if (linea) add_history(linea);
		char** lineaSpliteada = string_split(linea," ");
		printf("operacion es %s\n", lineaSpliteada[0]);
		parseoConsola(lineaSpliteada[0], lineaSpliteada[1]);
		free(linea);
	}
}

void parseoConsola(char* operacion, char* primerParametro) {
	int pid;
	if (string_equals_ignore_case (operacion, EJECUTAR)) {
		printf("path a ejecutar es %s\n", primerParametro);
		ejecutar(primerParametro);
	} else if (string_equals_ignore_case (operacion, STATUS)) {
		if (primerParametro != NULL) {
			pid = atoi(primerParametro);
			printf("pid a mostrar status es %i\n", pid);
			//mostrarDatosDe(pid);
		} else {
			printf("status no trajo parametros. Se mostraran estados de las colas\n");
			//mostrarEstadosDeColas();
		}
		printf ("primerParametro es: %s\n", primerParametro);
	} else if(string_equals_ignore_case (operacion, FINALIZAR)) {
		pid = atoi(primerParametro);
		printf("pid a finalizar es %i\n", pid);
		//moverAExit(pid);
	} else if(string_equals_ignore_case (operacion, METRICAS)) {
		if (primerParametro != NULL) {
			pid = atoi(primerParametro);
			printf("pid a mostrar metricas es %i\n", pid);
			//mostrarMetricasDe(pid);
		} else {
			printf("metricas no trajo parametros. Solo se muestran estadisticas del sistema\n");
		}
		//mostrarMetricasDelSistema();
	} else printf("No se conoce la operacion\n");
}

void accion(void* socket) {
	int socketFD = *(int*) socket;
	Paquete paquete;
	void* datos;
	//while que recibe paquetes que envian a safa, de qué socket y el paquete mismo
	//el socket del cliente conectado lo obtiene con la funcion servidorConcurrente en el main
	while (RecibirPaqueteServidorSafa(socketFD, SAFA, &paquete) > 0) {
		switch(paquete.header.emisor) {
			case ELDIEGO: {

				switch (paquete.header.tipoMensaje) {
					case ESHANDSHAKE: {
                    	pthread_mutex_unlock(&mutex_handshake_diego);
						log_info(logger, "llegada de el diego");
						break;
					}
				}
			}

			case CPU: {
				manejar_paquetes_CPU(&paquete, &socketFD);
				break;
			}
		}
	}
	if (paquete.Payload != NULL) free(paquete.Payload);
	close(socketFD);
}

void manejar_paquetes_CPU(Paquete* paquete, int* socketFD) {
	switch (paquete->header.tipoMensaje) {
		case ESHANDSHAKE: {
			t_cpu *cpu_nuevo = malloc(sizeof(t_cpu));
			cpu_nuevo->socket = *socketFD;
            pthread_mutex_init(&cpu_nuevo->mutex_estado, NULL);
			list_add(lista_cpu, cpu_nuevo);
			log_info(logger, "llegada cpu nuevo en socket: %i", cpu_nuevo->socket);
            pthread_mutex_unlock(&mutex_handshake_cpu);
			break;
		}

    	case DUMMY_SUCCES: {
            u_int32_t pid;
            memcpy(&pid, paquete->Payload, sizeof(u_int32_t));
            DTB = list_remove_by_condition(lista_bloqueados, (void*)coincide_pid(&pid));
			notificar_al_PLP(lista_nuevos, &pid);
            t_cpu* cpu_actual = devuelve_cpu_asociada_a_socket_de_lista(lista_cpu, socketFD);
            pthread_mutex_unlock(&cpu_actual->mutex_estado);
			break;
		}

        case DUMMY_FAIL: {
			break;
        }

        case DTB_BLOQUEAR: {
			break;
    	}

        case PROCESS_TIMEOUT: {
        	// falta quantum. Chequeo si finaliza o vuelve a listo aca y en succes
			break;
        }

        case DTB_SUCCES: {
            DTB* DTB_succes = DTB_deserializar(paquete->Payload);
            int pid = DTB_succes->gdtPID;
            // list_remove_by_condition(lista_bloqueados, (void*)coincide_pid(&pid));
			// chequear si va a ready o a exit
            t_cpu* cpu_actual = devuelve_cpu_asociada_a_socket_de_lista(lista_cpu, socketFD);
			pthread_mutex_unlock(&cpu_actual->mutex_estado);
			break;
        }
	}
}

int main(void) {
	crearLogger();
	inicializarVariables();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	log_info(logger, "Probando SAFA.log");
	//defino a safa como servidor concurrente para que diego y cpu se puedan conectar a él
	//a los clientes conectados a safa se los atiende con un hilo y mediante la funcion accion a c/u
    ServidorConcurrente(IP, PUERTO, SAFA, &lista_hilos, &end, accion);
    
    pthread_mutex_lock(&mutex_handshake_diego);
    pthread_mutex_lock(&mutex_handshake_cpu);
    pthread_t hiloConsola; //un hilo para la consola
    pthread_create(&hiloConsola, NULL, (void*) consola, NULL);

    pthread_t hilo_PLP;
    pthread_create(&hilo_PLP, NULL, (void*) planificador_largo_plazo, NULL);

    pthread_t hilo_PCP;
    pthread_create(&hilo_PCP, NULL, (void*) planificador_corto_plazo, NULL);

	pthread_join(hiloConsola, NULL);
    pthread_join(hilo_PCP, NULL);
    pthread_join(hilo_PLP, NULL);
	return EXIT_SUCCESS;
}
