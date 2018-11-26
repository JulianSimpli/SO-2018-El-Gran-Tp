#include "fm9.h"

char *MODO, *IP_FM9;
int PUERTO_FM9, TAMANIO, MAX_LINEA, TAM_PAGINA, socketElDiego, lineasTotales, framesTotales, lineasPorFrame;
char** memoria;

t_list* listaHilos;
t_list* framesMemoria;
t_list* procesos;

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

/* primitivas/tipoMensaje cpu
 * nueva primitiva: recibo processId; PC
 * asignar: recibo processId, path, posicion de la linea de tal path, dato (tengo que escrbiri en la linea)
 * close: recibo PID, path
 *
 * primitivas Diego
 * abrir: el dam envia de a partes (recibir de a partes)
 * flush: envio archivo modificado
 */

void enviarOperacionOK(int Receptor) {

}

char* lineaDeUnaPosicion(int pid, int pc) {
	int pcReal = pc-1; //porque la linea 1 del archivo esta guardada en la posicion 0
	ProcesoArchivo* unProceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
	SegmentoArchivoSEG* unSegmento = list_get(unProceso->segmentos, 0);
	if(pcReal > unSegmento->fin) {
		//segmentation fault
		return NULL;
	} else
		return ((char*) list_get(unSegmento->lineas, pcReal));
}

void accion(void* socket) {
	int socketFD = *(int*) socket;
	Paquete paquete;
	void* datos;
	while (RecibirPaqueteServidorFm9(socketFD, FM9, &paquete) > 0) {
		if (paquete.header.emisor == ELDIEGO) {
			switch (paquete.header.tipoMensaje) {
				case ABRIR: {
					if(!strcmp(MODO, "SEG")) {
						//el payload debe ser: int(id)+char*(path)\0+char*(archivo)
						int pid;
						memcpy(&pid, paquete.Payload, sizeof(pid));
						paquete.Payload += sizeof(pid);
						char* fid = malloc(strlen(paquete.Payload)+1);
						strcpy(fid, paquete.Payload);
						paquete.Payload += strlen(fid)+1;
						char* file = malloc(strlen(paquete.Payload)+1);
						strcpy(file, paquete.Payload);
						cargarArchivoAMemoriaSEG(pid, fid, file);
					} else if(!strcmp(MODO, "TPI")) {

					} else if(!strcmp(MODO, "SPA")) {

					}
				}
				break;
				case FLUSH: {

				}
				break;
			}
		}
		if (paquete.header.emisor == CPU) {
			switch (paquete.header.tipoMensaje) {
				case NUEVA_PRIMITIVA: {
					//el payload debe ser: int(pid)+int(pc)
					int pid, pc;
					memcpy(&pid, paquete.Payload, sizeof(pid));
					paquete.Payload =+ sizeof(pid);
					memcpy(&pc, paquete.Payload, sizeof(pc));
					char* linea = malloc(MAX_LINEA);
					linea = lineaDeUnaPosicion(pid, pc);
					linea = realloc(linea, strlen(linea)+1);
					datos = linea;
					EnviarDatosTipo(socketFD, CPU, datos, sizeof(linea), LINEA_PEDIDA);
				}
				break;
				case ASIGNAR: {

				}
				break;
				case CLOSE: {

				}
				break;
			}
		}
	}
}

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
	procesos = list_create();
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

/*
 * devuelve el nro de linea en el que empiezan los espacios libres consecutivos,
 * en caso de no hallar devuelve -1
 *
 * Ésta funcion se debe a que un segmento está determinado por varias lineas y un segmento ocupa un espacio
 * consecutivo de lineas, el mismo no se puede definir en partes, es uno.
 */
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

int contarElementosArray(char** array) {
	int contador = 0;
	while (array[contador] != NULL) {
		contador++;
	}
	return contador;
}

void cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo) {
	char** arrayLineas = string_split(archivo, "\n"); //deja un elemento con NULL al final
	int cantidadDeLineas = contarElementosArray(arrayLineas);
	log_info(logger, "El archivo a cargar es...");
	int inicioDeLinea = lineasLibresConsecutivasMemoria(cantidadDeLineas);
	if (inicioDeLinea >= 0) {
		//me fijo si ya tengo el proceso en lista y solo le agrego el archivo a su lista de segmentos
		if (list_any_satisfy(procesos, LAMBDA(bool _(ProcesoArchivo* proceso){return proceso->idProceso == idProceso;}))) {
			ProcesoArchivo* unProceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == idProceso;}));
			SegmentoArchivoSEG* nuevoSegmento = malloc(sizeof(SegmentoArchivoSEG));
			nuevoSegmento->idArchivo = malloc(strlen(path) + 1);
			strcpy(nuevoSegmento->idArchivo, path);
			t_list* lineasArchivo = list_create();
			for (int x = 0; x < cantidadDeLineas; x++) {
				list_add(lineasArchivo, arrayLineas[x]);
			}
			nuevoSegmento->lineas = lineasArchivo;
			nuevoSegmento->inicio = inicioDeLinea;
			nuevoSegmento->fin = list_size(lineasArchivo) - 1;
			list_add(unProceso->segmentos, nuevoSegmento);
			for(int x = 0; x < cantidadDeLineas; x++) {
				strcpy(memoria[inicioDeLinea], arrayLineas[x]);
				inicioDeLinea++;
			}
		} else {
			ProcesoArchivo* procesoArchivo = malloc(sizeof(procesoArchivo));
			procesoArchivo->idProceso = idProceso;
			t_list* segmentos = list_create();
			SegmentoArchivoSEG* unSegmentoArchivoSEG = malloc(sizeof(SegmentoArchivoSEG));
			unSegmentoArchivoSEG->idArchivo = malloc(strlen(path) + 1);
			strcpy(unSegmentoArchivoSEG->idArchivo, path);
			t_list* lineasArchivo = list_create();
			for (int x = 0; x < cantidadDeLineas; x++) {
				list_add(lineasArchivo, arrayLineas[x]);
			}
			unSegmentoArchivoSEG->lineas = lineasArchivo;
			unSegmentoArchivoSEG->inicio = inicioDeLinea;
			unSegmentoArchivoSEG->fin = list_size(lineasArchivo) - 1;
			list_add(segmentos, unSegmentoArchivoSEG);
			procesoArchivo->segmentos = segmentos;
			list_add(procesos, procesoArchivo);
			for(int x = 0; x < cantidadDeLineas; x++) {
				strcpy(memoria[inicioDeLinea], arrayLineas[x]);
				inicioDeLinea++;
			}
		}
		log_info(logger, "Archivo cargado con éxito...");
	} else {
		log_info(logger, "No hay espacio suficiente para cargar tal segmento...");
	}
}

void imprimirMemoria(){
	for(int i = 0; i < lineasTotales; i++) {
		printf("\nposicion de memoria: linea %d, dato: %s", i, memoria[i]);
	}
	printf("\ncantidad lineas libres de memoria: %d\n", cantidadLineasLibresMemoria());
}

void modoDeEjecucion(char* modo) {
	//“SEG” - “TPI” - “SPA”
	if(!strcmp(modo, "SEG")) {

	} else if(!strcmp(modo, "TPI")) {

	} else if(!strcmp(modo, "SPA")) {
		framesTotales = TAMANIO/TAM_PAGINA;
		lineasPorFrame = lineasTotales/framesTotales; //lineasPorFrame= lineasPorPagina

	}
}

void inicializarFramesMemoria() {
	framesMemoria = list_create();
	int inicio = 0;
	for(int x = 0; x < (lineasTotales/lineasPorFrame); x++) {
		Frame* frame = malloc(sizeof(Frame));
		frame->disponible = 1;
		frame->inicio = inicio;
		frame->fin = inicio + lineasPorFrame - 1;
		frame->lineas = list_create();
		list_add(framesMemoria, frame);
		inicio += lineasPorFrame;
	}
}

int framesSuficientes(int lineasAGuardar) {
	int flag = 0;
	bool disponible(void* frame) {
		return ((Frame*)frame)->disponible == 1;
	}
	int lineasDisponibles = lineasPorFrame * list_count_satisfying(framesMemoria, disponible);
	if(lineasDisponibles >= lineasAGuardar) {
		flag = 1;
	}
	return flag;
}

int primerFrameLibre() {
	int posicion = 0;
	for(int x = 0; x < list_size(framesMemoria); x++) {
		Frame* frame = (Frame*) list_get(framesMemoria, x);
		if(frame->disponible) {
			posicion = -1;
		} else {
			posicion = x;
			break;
		}
	}
	return posicion;
}

void persistirArchivoEnPaginas() {
	//supongo:
	//un archivo de 15 lineas
	//5 frames totales de memoria principal de 5 lineas cada uno
	//por ende tengo 1 segmento de 15 lineas que debo guardar en 3 frames
	int idProc = 1;
	char* idArch = "path";
	char* archivo = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15";
	int cantidadLineasArchivo = 15;
	int framesTotales = 5;
	int lineasPorframe = 5;
	//-----------------------
	char** arrayLineas = string_n_split(archivo, cantidadLineasArchivo, "\n");
	if(framesSuficientes(cantidadLineasArchivo)) {
		ProcesoArchivo* procesoArchivo = malloc(sizeof(ProcesoArchivo));
		procesoArchivo->idProceso = idProc;
		t_list* segmentos = list_create();
		SegmentoArchivoSPA* SegmentoArchivoSPA = malloc(sizeof(SegmentoArchivoSPA));
		SegmentoArchivoSPA->idArchivo = malloc(strlen(idArch)+1);
		strcpy(SegmentoArchivoSPA->idArchivo, idArch);
		t_list* paginas = list_create();
		int cantidadPaginasAUsar = ceil(cantidadLineasArchivo/lineasPorframe); //si rompe, ver agregar -lm math.h o 'm' a libraries (eclipse)
		SegmentoArchivoSPA->cantidadPaginas = cantidadPaginasAUsar;
		int lineas = 0;
		for(int x = 0; x < cantidadPaginasAUsar; x++) {
			Frame* frame = (Frame*) list_get(framesTotales, primerFrameLibre());
			int inicioFrame = frame->inicio;
			while(lineas < cantidadLineasArchivo || inicioFrame <= frame->fin) {
				list_add(frame->lineas, arrayLineas[lineas]);
				strcpy(memoria[inicioFrame], arrayLineas[lineas]);
				inicioFrame++;
				lineas++;
			}
			frame->disponible = 0;
			list_add(paginas, frame);
		}
		SegmentoArchivoSPA->paginas = paginas;
		list_add(segmentos, SegmentoArchivoSPA);
		list_add(procesos, procesoArchivo);
	} else {
		//no hay espacio ):
	}
}

int main() {
	crearLogger();
	log_info(logger, "Probando FM9.log");
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	asignarMemoriaLineas();
	imprimirMemoria();
	ServidorConcurrente(IP_FM9, PUERTO_FM9, FM9, &listaHilos, &end, accion);
	pthread_t hiloConsola; //un hilo para la consola
	pthread_create(&hiloConsola, NULL, (void*) consola, NULL);
	pthread_join(hiloConsola, NULL);
	return EXIT_SUCCESS;
}
