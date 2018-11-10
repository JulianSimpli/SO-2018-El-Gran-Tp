#include "safa.h"

/*Creación de Logger*/
void crearLogger() {
	logger = log_create("safa.log", "safa", true, LOG_LEVEL_INFO);
}

void inicializarVariables() {

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
}

void llenar_lista_estados()
{
	list_add_in_index(lista_estados, 0, lista_nuevos);
	list_add_in_index(lista_estados, 1, lista_listos);
	list_add_in_index(lista_estados, 2, lista_ejecutando);
	list_add_in_index(lista_estados, 3, lista_bloqueados);
	list_add_in_index(lista_estados, 4, lista_finalizados);
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("/home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/src/SAFA.config");
	IP = "127.0.0.1";
	PUERTO = config_get_int_value(arch, "PUERTO");
	ALGORITMO_PLANIFICACION = string_duplicate(config_get_string_value(arch, "ALGORITMO"));
	QUANTUM = config_get_int_value(arch, "QUANTUM");
	MULTIPROGRAMACION = config_get_int_value(arch, "MULTIPROGRAMACION");
	RETARDO_PLANIF = config_get_int_value(arch, "RETARDO_PLANIF");
	config_destroy(arch);
}

void imprimirArchivoConfiguracion() {
	printf("Configuración:\n"
			"IP=%s\n"
			"PUERTO=%d\n"
			"ALGORITMO_PLANIFICACION=%s\n"
			"QUANTUM=%d\n"
			"MULTIPROGRAMACION=%d\n"
			"RETARDO_PLANIF=%d\n",
			IP, PUERTO, ALGORITMO_PLANIFICACION, QUANTUM, MULTIPROGRAMACION, RETARDO_PLANIF);
}

void consola() {
	printf("Esperando que conecte el diego y al menos 1 CPU\nSAFA esta en estado corrupto\n");
	log_info(logger, "Esperando que conecte el diego y al menos 1 CPU. SAFA esta en estado corrupto\n");
	// sem_wait(&mutex_handshake_diego);
	// sem_wait(&mutex_handshake_cpu);
	printf("Se conectaron Diego y 1 CPU\nSAFA esta en estado operativo\nYa puede usar la consola\n");
	log_info(logger, "Se conectaron Diego y 1 CPU. SAFA esta en estado operativo");

	char *linea;
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
	int pid = 0;
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
		finalizar(&pid);
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

void accion(void* socket)
{
	int socketFD = *(int*) socket;
	Paquete paquete;
	void* datos;
	//while que recibe paquetes que envian a safa, de qué socket y el paquete mismo
	//el socket del cliente conectado lo obtiene con la funcion servidorConcurrente en el main
	while (RecibirPaqueteServidorSafa(socketFD, SAFA, &paquete) > 0)
	{
		switch(paquete.header.emisor)
		{
			case ELDIEGO:
			{
				manejar_paquetes_diego(&paquete, &socketFD);
				break;
			}
			case CPU:
			{
				manejar_paquetes_CPU(&paquete, &socketFD);
				break;
			}
		}
	}
	if (paquete.Payload != NULL) free(paquete.Payload);
	close(socketFD);
}

void manejar_paquetes_diego(Paquete *paquete, int *socketFD)
{
	switch (paquete->header.tipoMensaje)
	{
		case ESHANDSHAKE:
		{
			sem_post(&mutex_handshake_diego);
			log_info(logger, "llegada de el diego");
			enviar_handshake_diego(*socketFD);
			break;
		}
		case DUMMY_SUCCES:
		{
			// Mensaje contiene pid, posicion en memoria, path
			u_int32_t pid;
			memcpy(&pid, paquete->Payload, sizeof(u_int32_t));
			log_info(logger, "%d fue cargado en memoria", pid);
			bloquear_dummy(&pid);
			notificar_al_plp(&pid);
			log_info(logger, "%d pasa a lista de procesos listos", pid);
			liberar_cpu(socketFD);
			log_info(logger, "Cpu %d liberada", *socketFD);
			break;
		}
		case DUMMY_FAIL:
		{
			u_int32_t pid;
			memcpy(&pid, paquete->Payload, sizeof(u_int32_t));
			bloquear_dummy(&pid);
			log_error(logger, "Fallo la carga en memoria del %d", pid);
			break;
		}
		case DTB_SUCCES:
		{
			break;
			//Me manda pid, archivos abiertos, datos de memoria virtual para buscar el archivo.
		}
		case DTB_FAIL:
		{
			break;
		}
	}
}

void manejar_paquetes_CPU(Paquete* paquete, int* socketFD) {
	switch (paquete->header.tipoMensaje) {
		case ESHANDSHAKE: {
			t_cpu *cpu_nuevo = malloc(sizeof(t_cpu));
			cpu_nuevo->socket = *socketFD;
            cpu_nuevo->estado = CPU_LIBRE;
			list_add(lista_cpu, cpu_nuevo);
			log_info(logger, "llegada cpu con id %i", cpu_nuevo->socket);
            sem_post(&mutex_handshake_cpu);
			enviar_handshake_cpu(*socketFD);
			break;
		}

        case DTB_BLOQUEAR: {
			break;
    	}

        case PROCESS_TIMEOUT: {
        	// falta quantum. Chequeo si finaliza o vuelve a listo aca y en succes
			break;
        }

        case DTB_EJECUTO: {
            DTB* DTB_succes = DTB_deserializar(paquete->Payload);
            int pid = DTB_succes->gdtPID;
            // list_remove_by_condition(lista_bloqueados, (void*)coincide_pid(&pid));
			// chequear si va a ready o a exit
			liberar_cpu(socketFD);
			break;
        }
	}
}

void *handshake_cpu_serializar(int *tamanio_payload)
{
	void *payload = malloc(sizeof(QUANTUM));
	int desplazamiento = 0;
	int tamanio = sizeof(u_int32_t);
	memcpy(payload, &QUANTUM, tamanio);
	desplazamiento += tamanio;

	u_int32_t len_algoritmo = strlen(ALGORITMO_PLANIFICACION);
	payload = realloc(payload, desplazamiento + tamanio);
	memcpy(payload + desplazamiento, &len_algoritmo, tamanio);
	desplazamiento += tamanio;

	payload = realloc(payload, desplazamiento + len_algoritmo);
	memcpy(payload + desplazamiento, ALGORITMO_PLANIFICACION, len_algoritmo);
	desplazamiento += len_algoritmo;

	*tamanio_payload = desplazamiento;
	return payload;
}

void enviar_handshake_cpu(int socketFD) {
	int tamanio_payload = 0;
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->Payload = handshake_cpu_serializar(&tamanio_payload);
	cargar_header(&paquete, tamanio_payload, ESHANDSHAKE, SAFA);
	EnviarPaquete(socketFD, paquete);
	free(paquete);
}

void enviar_handshake_diego(int socketFD) {
	Paquete* paquete = malloc(sizeof(Paquete));
	cargar_header(&paquete, 0, ESHANDSHAKE, SAFA);

	EnviarPaquete(socketFD, paquete);
	free(paquete);
}

int main(void) {
	crearLogger();
	inicializarVariables();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	log_info(logger, "Probando SAFA.log");

	pthread_t hiloConsola;
    pthread_create(&hiloConsola, NULL, (void*) consola, NULL);

    pthread_t hilo_plp;
    pthread_create(&hilo_plp, NULL, (void*) planificador_largo_plazo, NULL);

    pthread_t hilo_pcp;
    pthread_create(&hilo_pcp, NULL, (void*) planificador_corto_plazo, NULL);

    ServidorConcurrente(IP, PUERTO, SAFA, &lista_hilos, &end, accion);

	pthread_join(hiloConsola, NULL);
    pthread_join(hilo_pcp, NULL);
    pthread_join(hilo_plp, NULL);
	return EXIT_SUCCESS;
}
