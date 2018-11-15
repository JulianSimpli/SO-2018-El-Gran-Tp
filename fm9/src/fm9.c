#include "../../Bibliotecas/sockets.h"


char *MODO, *IP_FM9;
int PUERTO_FM9, TAMANIO, MAX_LINEA, TAM_PAGINA, socketElDiego, lineasTotales;
char** memoria;

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

void accion(void* socket) {
	int socketFD = *(int*) socket;
	Paquete paquete;
	void* datos;
	while (RecibirPaqueteServidorFm9(socketFD, FM9, &paquete) > 0) {
		int emisor = paquete.header.emisor;
//		if (paquete.header.emisor = ELDIEGO) {
//			switch (paquete.header.TipoMensaje) {
//				case MENSAJE: {
//				}
//				break;
//			}
//		}
	}
}

/*
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
		if (!strcmp(dump[0], "dump")) {
			int idDump = atoi(dump[1]);
			printf ("id para Dump: %d", idDump);
			//mostrar toda la info del id que pertenece a un proceso, y la info del storage
		}
		else printf("No se conoce el comando\n");
		free(linea);
	}
}

int RecibirPaqueteServidorFm9(int socketFD, Emisor receptor, Paquete* paquete) {
	paquete->Payload = NULL;
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	char* n = Emisores[paquete->header.emisor];
	if (resul > 0) { //si no hubo error
		if ((paquete->header.tipoMensaje == ESHANDSHAKE) && (paquete->header.emisor == ELDIEGO)) { //vemos si es un handshake
			printf("Se establecio conexion con %s\n", n);
			EnviarHandshakeELDIEGO(socketFD); // paquete->header.emisor
		} else if ((paquete->header.tipoMensaje == ESHANDSHAKE) && (paquete->header.emisor == CPU)) {
			printf("Se establecio conexion con %s\n", n);
			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
		} else if (paquete->header.tamPayload > 0){ //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = malloc(paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}
	return resul;
}

void EnviarHandshakeELDIEGO(int socketFD) {
	Paquete* paquete = malloc(TAMANIOHEADER + sizeof(MAX_LINEA));
	Header header;
	header.tipoMensaje = ESHANDSHAKE;
	header.tamPayload = sizeof(MAX_LINEA);
	header.emisor = FM9;
	paquete->header = header;
	paquete->Payload = malloc(sizeof(MAX_LINEA));
	memcpy(paquete->Payload, &MAX_LINEA, sizeof(u_int32_t));
	bool valor_retorno=EnviarPaquete(socketFD, paquete);
	free(paquete);
}

/*
typedef struct DTB{
	u_int32_t gdtPID; // me importa
	u_int32_t PC; // me importa
	u_int32_t flagInicializacion;
	char *pathEscriptorio;
	t_list *archivosAbiertos; // me importa
} DTB;
*/

void asignarMemoriaLineas() {
	lineasTotales = TAMANIO/MAX_LINEA;
	memoria = (char**) malloc(lineasTotales * sizeof(char*));
	for(int i = 0; i < lineasTotales; i++) {
		memoria[i] = (char*) malloc(MAX_LINEA);
		strcpy(memoria[i], "NaN");
	}
}

int cantidadLineasLibresMemoria() {
	int contador = 0;
	for(int i = 0; i < lineasTotales; i++){
		if(!strcmp(memoria[i], "NaN")){
			contador++;
		}
	}
	return contador;
}

//devuelve el nro de linea en el que empiezan los espacios libres consecutivos
int lineasLibresConsecutivasMemoria(int lineasAGuardar) {
	int contador = 0;
	for(int i = 0; i < lineasTotales; i++){
		if(!strcmp(memoria[i], "NaN")){
			contador++;
		} else {
			contador = 0;
		}
		if(contador == lineasAGuardar){
			return (i-contador+1);
		}
	}
	return -1;
}

int primerLineaVaciaMemoria() {
	for(int i = 0; i < lineasTotales; i++){
		if(!strcmp(memoria[i], "NaN")){
			return i;
		}
	}
	return -1;
}

void ejemploCargarArchivoAMemoria() {
	//recibo el archivo en string(?), cantidad de lineas (o las cuento yo), un id del archivo
	//y el id del proceso que lo abrió
	//------------
	int idProc = 1;
	char* idArch = "path";
	char* archivo = "primer linea\nsegunda linea\ntercera linea";
	int cantidadLineas = 3;
	//-----------
	//separo el string en lineas separando por \n
	char** arrayLineas = string_n_split(archivo, cantidadLineas, "\n");
	printf("\nguardo un archivo que contiene las siguientes lineas:\n%s\n%s\n%s\n", arrayLineas[0], arrayLineas[1], arrayLineas[2]);
	//hay espacio en memoria?
	int lineasLibres = lineasLibresConsecutivasMemoria(cantidadLineas);
	if (lineasLibres>=0) {
		//inicializo estructuras
		ProcesoArchivo* procesoArchivo = malloc(sizeof(procesoArchivo));
		t_list* segmentos;
		segmentos = list_create();
		SegmentoArchivo* unSegmentoArchivo = malloc(sizeof(SegmentoArchivo));
		t_list* lineasArchivo;
		lineasArchivo = list_create();
		unSegmentoArchivo->idArchivo = malloc(strlen(idArch) + 1);
		//persistencia de datos
		strcpy(unSegmentoArchivo->idArchivo, idArch);
		for (int x = 0; x < cantidadLineas; x++) {
			list_add(lineasArchivo, arrayLineas[x]);
		}
		//guardo las lineas de un archivo-segmento en una lista
		unSegmentoArchivo->lineas = lineasArchivo;
		procesoArchivo->idProceso = idProc;
		list_add(segmentos, unSegmentoArchivo);
		procesoArchivo->segmentos = segmentos;
		//hasta acá debería tener guardado un id de proceso con su lista de segmentos
		//cuya lista de segmentos posee 1 elemento el cual tiene un id de archivo y su lista de lineas
		//me falta agregarle el inicio de la ubicacion en memoria real de cada archivo
		//cargo a memoria real cada segmento
		unSegmentoArchivo->inicio = lineasLibres;
		int linea = 0;
		for(int x = lineasLibres; x < (cantidadLineas+lineasLibres); x++) {
			strcpy(memoria[x], arrayLineas[linea]);
			linea++;
		}
	}
}

void imprimirMemoria(){
	for(int i = 0; i < lineasTotales; i++) {
		printf("\nposicion de memoria: linea %d, dato: %s", i, memoria[i]);
	}
	printf("\ncantidad lineas libres de memoria: %d\n", cantidadLineasLibresMemoria());
}

int main() {
	crearLogger();
	log_info(logger, "Probando FM9.log");
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	asignarMemoriaLineas();
	imprimirMemoria();
	ejemploCargarArchivoAMemoria();
	imprimirMemoria();
	ServidorConcurrente(IP_FM9, PUERTO_FM9, FM9, &listaHilos, &end, accion);
	pthread_t hiloConsola; //un hilo para la consola
	pthread_create(&hiloConsola, NULL, (void*) consola, NULL);
	pthread_join(hiloConsola, NULL);
	return EXIT_SUCCESS;
}
