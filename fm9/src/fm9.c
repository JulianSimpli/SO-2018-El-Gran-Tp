#include "../../Bibliotecas/sockets.h"

char *MODO, *IP_FM9;
int PUERTO_FM9, TAMANIO, MAX_LINEA, TAM_PAGINA, socketElDiego;
void *ptrStorage;

t_list* listaHilos;
bool end;

t_log* logger;
/*Creación de Logger*/
void crearLogger() {
	logger = log_create("FM9.log", "FM9", true, LOG_LEVEL_INFO);
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("/home/utnso/workspace/tp-2018-2c-Nene-Malloc/fm9/src/FM9.config");

	IP_FM9 = string_duplicate(config_get_string_value(arch, "IP_FM9"));
	PUERTO_FM9 = config_get_int_value(arch, "PUERTO_FM9");
	MODO = string_duplicate(config_get_string_value(arch, "MODO"));
	TAMANIO = config_get_int_value(arch, "TAMANIO");
	MAX_LINEA = config_get_int_value(arch, "MAX_LINEA");
	TAM_PAGINA = config_get_int_value(arch, "TAM_PAGINA");

	config_destroy(arch);
}

void imprimirArchivoConfiguracion() {
	printf("Configuracion:\n"
			"IP_FM9=%s\n"
			"PUERTO_FM9=%d\n"
			"MODO=%s\n"
			"TAMANIO=%d\n"
			"MAX_LINEA=%d\n"
			"TAMANIO_PAGINA=%d\n",
			IP_FM9, PUERTO_FM9, MODO, TAMANIO, MAX_LINEA, TAM_PAGINA);
}

void EnviarHandshakeELDIEGO(int socketFD) {
	Paquete* paquete = malloc(TAMANIOHEADER);
	Header header;
	void *datos = &MAX_LINEA;
	header.tipoMensaje = ESHANDSHAKE;
	header.tamPayload = sizeof(MAX_LINEA);
	header.emisor = FM9;
	paquete->header = header;
	paquete->Payload = datos;
	bool valor_retorno=EnviarPaquete(socketFD, paquete);
	free(paquete);
}

void accion(void* socket) {
	int socketFD = *(int*) socket;
	Paquete paquete;
	void* datos;
	while (RecibirPaqueteServidor(socketFD, FM9, &paquete) > 0) {
		int emisor = paquete.header.emisor;
		if (paquete.header.emisor = ELDIEGO) {
			switch (paquete.header.tipoMensaje) {
				case ESHANDSHAKE: {
					socketElDiego = socketFD;
					//otra opcion mas sencilla, usar directamente:
					//bool EnviarDatosTipo(int socketFD, char emisor[8], void* datos, int tamDatos, tipo tipoMensaje);
					//EnviarDatosTipo(socketFD, FM9, MAX_LINE(debe ser puntero al int), sizeof(MAX_LINE), ESHANDSHAKE);
					EnviarHandshakeELDIEGO(socketFD);
				}
				break;
			}
		}
	}
}

/*
Estructuras a utilizar:

¡DTB! el cual debe contener:
	id proceso
	socket
	tamaño del archivo (cantidad de lineas)
	inicio del segmento
	desplazamiento
(si el proceso se divide en stack, codigo, lo que sea;
tiene que haber un inicio del segmento y desplazamiento para cada uno)

le estructura que reciba de MDJ al 'abrir' debe contener:
	un proceso (DTB)
	y el contenido de la ruta que se mandó a abrir
(Respecto a mdj tener en cuenta el transfer size)

typedef struct {
	int socket;
	int idProceso;
	t_list* segmentos;
} procesoSegmentacion;

typedef struct {
	int nroSegmento;
	int inicio;
	int desplazamiento;
} segmentacion;

typedef struct {
	void* inicio;

} storage;

typedef struct {
	t_list* t_segmentosProceso;
	int tamanio;
typedef struct {
	cpu;
	script;
} proceso;

en la funcion accion (tener en cuenta siempre transfer size -> buffering):
while (RecibirPaqueteServidor(socketFD, FM9, &paquete) > 0) {
	if (!strcmp(paquete.header.emisor, ELDIEGO)) {
		switch (paquete.header.tipoMensaje) {
			case OPERACIONABRIR: {

				}
				break;

			case OPERACIONFLUSH: {

				}
				break;
		}
	}
}



*/

void consola() {
	char * linea;
	while (true) {
		linea = readline(">> ");
		if (linea) add_history(linea);
		char** dump = string_split(linea, " ");
		if (string_starts_with(linea, dump[0])) {
			int idDump = atoi(dump[1]);
			printf ("id para Dump: %d", idDump);
			//mostrar toda la info del id que pertenece a un proceso, y la info del storage
		}
		else printf("No se conoce el comando\n");
		free(linea);
	}
}

int main(int argc, char **argv) {
	crearLogger();
	log_info(logger, "Probando FM9.log");
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	//ptrStorage = malloc(TAMANIO);
	pthread_t hiloConsola; //un hilo para la consola
	pthread_create(&hiloConsola, NULL, (void*) consola, NULL);
	ServidorConcurrente(IP_FM9, PUERTO_FM9, FM9, &listaHilos, &end, accion);
	pthread_join(hiloConsola, NULL);
	return EXIT_SUCCESS;
}
