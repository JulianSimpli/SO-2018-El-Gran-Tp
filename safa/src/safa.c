#include "safa.h"

/*Creación de Logger*/
void crearLogger() {
	logger = log_create("safaLog.log", "safa", true, LOG_LEVEL_INFO);
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
	t_config* arch = config_create("/home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/safa.cfg");
	IP = string_duplicate(config_get_string_value(arch, "IP")); //agregado
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
    printf("Esperando 1 CPU y a El Diego");
    while(estado);
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
		//SWITCH
		if (!strcmp(paquete.header.emisor, ELDIEGO)) {
			switch (paquete.header.tipoMensaje) {
				case ESHANDSHAKE: { //tipo de mensaje y la acción que debe realizar segun el mensaje enviado
                    pthread_mutex_unlock(&mutex_handshake_diego);
				}
				break;
			}
		}
		if (!strcmp(paquete.header.emisor, CPU)) {
			switch (paquete.header.tipoMensaje) {
				//recibio un mensaje de tipo "handshake" de un cpu, el cual a su vez le pasa el id de cpu conectado
				case ESHANDSHAKE: {
					t_unCpuSafa *cpuNuevo = malloc(sizeof(t_unCpuSafa));
					cpuNuevo->id = malloc(paquete.header.tamPayload);
					strcpy(cpuNuevo->id, paquete.Payload);
					cpuNuevo->socket = socketFD;
                    pthread_mutex_init(&(cpuNuevo->mutex_estado));
					list_add(lista_cpu, cpuNuevo);
					log_info(logger, "llegada cpu nuevo con id: %s", cpuNuevo->id);
                    pthread_mutex_unlock(&mutex_handshake_cpu);
				}

                case DUMMY_SUCCES: {
                    int pid = (int*) paquete.Payload;
                    list_remove_by_condition(lista_bloqueados, (void*)coincidePID(&pid));
                    notificar_al_PLP(pid, lista_nuevos);
                    t_unCpuSafa* cpu_actual= list_find(lista_cpu, (void*)coincide_socket(&SocketFD));
                    pthread_muted_unlock(cpu_actual->mutex_estado);
                }

                case DUMMY_FAIL: {

                }

                case BLOQUEAR_GDT: {

                }

                case PROCESS_TIMEOUT: {
                    // falta quantum. Chequeo si finaliza o vuelve a listo aca y en succes
                }

                case DTB_SUCCES: {
                    DTB* DTB_succes = deserializar(paquete.Payload);
                    int pid = DTB_succes->gdtPID;
                    list_remove_by_condition(lista_bloqueados, (void*)coincidePID(&pid));
                    // chequear si va a ready o a exit
                    t_unCpuSafa* cpu_actual= list_find(lista_cpu, (void*)coincide_socket(&SocketFD));
                    pthread_muted_unlock(cpu_actual->mutex_estado);
                }

                    
			}
				break;
		}
	}
	if (paquete.Payload != NULL) free(paquete.Payload);
	close(socketFD);
}

int main(void) {
	crearLogger();
	inicializarVariables();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	log_info(logger, "Probando safaLog.log");
	//defino a safa como servidor concurrente para que diego y cpu se puedan conectar a él
	//a los clientes conectados a safa se los atiende con un hilo y mediante la funcion accion a c/u
    ServidorConcurrente(IP, PUERTO, SAFA, &listaHilos, &end, accion);
    
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
