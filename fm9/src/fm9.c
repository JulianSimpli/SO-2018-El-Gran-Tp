#include "fm9.h"
#include <commons/string.h>

char *MODO, *IP_FM9;
int PUERTO_FM9, TAMANIO, MAX_LINEA, TAM_PAGINA, socketElDiego, lineasTotales, framesTotales, lineasPorFrame, transfer_size;
char** memoria;

t_list* listaHilos;
t_list* framesMemoria;
t_list* procesos;
t_list* tpinvertida;

sem_t abrir;

bool end;

t_log* logger;

////////////////////////////////////////// ↓↓↓  INICIALIZAR ↓↓↓  //////////////////////////////////////////
void crearLogger() {
	logger = log_create("FM9.log", "FM9", true, LOG_LEVEL_DEBUG);
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("/home/utnso/tp-2018-2c-Nene-Malloc/fm9/src/FM9.config");
	IP_FM9 = string_duplicate(config_get_string_value(arch, "IP_FM9"));
	PUERTO_FM9 = config_get_int_value(arch, "PUERTO_FM9");
	MODO = (char*) string_duplicate(config_get_string_value(arch, "MODO"));
	TAMANIO = config_get_int_value(arch, "TAMANIO");
	MAX_LINEA = config_get_int_value(arch, "MAX_LINEA");
	TAM_PAGINA = config_get_int_value(arch, "TAM_PAGINA");
	config_destroy(arch);
}

void inicializarMemoriaLineas() {
	transfer_size = 0;
	sem_init(&abrir, 0, 1);
	procesos = list_create();
	lineasTotales = TAMANIO/MAX_LINEA;
	memoria = (char**) malloc(lineasTotales * sizeof(char*));
	for(int i = 0; i < lineasTotales; i++) {
		memoria[i] = (char*) malloc(MAX_LINEA);
		strcpy(memoria[i], "NaN");
	}
}

void inicializar(char* modo) {
	// MODO -> “SEG” - “TPI” - “SPA”
	printf("Inicializacion de memoria y estructuras para el modo: %s\n", modo);
	inicializarMemoriaLineas();
	printf("Memoria con espacio de %d lineas\n", lineasTotales);
	if(!strcmp(modo, "SPA") || !strcmp(modo, "TPI")) {
		framesTotales = TAMANIO/TAM_PAGINA;
		lineasPorFrame = lineasTotales/framesTotales; //lineasPorFrame = lineasPorPagina
		framesMemoria = list_create();
		int inicio = 0;
		for(int x = 0; x < framesTotales; x++) {
			Frame* frame = malloc(sizeof(Frame));
			frame->disponible = 1;
			frame->inicio = inicio;
			frame->fin = inicio + lineasPorFrame - 1;
			frame->lineas = list_create();
			list_add(framesMemoria, frame);
			inicio += lineasPorFrame;
		}
		printf("Frames totales: %d\nLineas por frame: %d\n", framesTotales, lineasPorFrame);
	}
	printf("Memoria y estructuras para el modo: %s inicializadas correctamente\n\n", modo);
}
////////////////////////////////////////// ↑↑↑  INICIALIZAR ↑↑↑  //////////////////////////////////////////

///////////////////////////////////// ↓↓↓ AUX ↓↓↓ ////////////////////////////////////////
bool existePID(int pid) {
	return list_any_satisfy(procesos, LAMBDA(bool _(ProcesoArchivo* proceso) {return proceso->idProceso == pid;}));
}

ProcesoArchivo* obtenerProcesoId(int pid) {
	return list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
}

SegmentoArchivoSEG* obtenerSegmentoSEG(t_list* lista, char* path) {
	return list_find(lista, LAMBDA(bool _(SegmentoArchivoSEG* s) {return !strcmp(s->idArchivo, path);}));
}

SegmentoArchivoSPA* obtenerSegmentoSPA(t_list* lista, char* path) {
	return list_find(lista, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
}
//////////////////////////////////// ↑↑↑ AUX ↑↑↑ /////////////////////////////////////////

///////////////////////////// ↓↓↓ LINEA DE UNA POSICION n ↓↓↓ //////////////////////////////
char* lineaDeUnaPosicionSPA(int pid, int pc) {
//	int pcReal = pc - 1;
	ProcesoArchivo* unProceso = obtenerProcesoId(pid);
	SegmentoArchivoSPA* segmento = list_get(unProceso->segmentos, 0);
	if(pc < segmento->cantidadLineas) {
		int calculoDePagina;
		if(pc) {
		calculoDePagina = ceil((float) pc/(float) lineasPorFrame);
		} else
			calculoDePagina = 0;
		int calculoDeLinea = pc - (lineasPorFrame * calculoDePagina);
		Pagina* pagina = list_get(segmento->paginas, calculoDePagina);
		Frame* frame = list_get(framesMemoria, pagina->numeroFrame);
		log_info(logger, "La DF es: frame %d + offset %d -> linea: %d", pagina->numeroFrame, pc, (frame->inicio + pc));
		return ((char*) list_get(frame->lineas, calculoDeLinea));
	} else {
		//segmentation fault
		return NULL;
	}
}

char* lineaDeUnaPosicionSEG(int pid, int pc) {
//	int pcReal = pc-1; //porque la linea 1 del archivo esta guardada en la posicion 0
	ProcesoArchivo* unProceso = obtenerProcesoId(pid);
	SegmentoArchivoSEG* unSegmento = list_get(unProceso->segmentos, 0);
	if(pc <= unSegmento->fin) {
		log_info(logger, "La DF es: posicion de inicio de segmento %d + offset %d -> linea: %d", unSegmento->inicio, pc, (unSegmento->inicio + pc));
		return ((char*) list_get(unSegmento->lineas, pc));
	} else
		//segmentation fault
		return NULL;
}
//////////////////////// ↑↑↑ LINEA DE UNA POSICION n ↑↑↑ //////////////////////////////

//////////////////////// ↓↓↓ PRIMITIVA ABRIR ↓↓↓ //////////////////////////////////////
char** cargarArray(char* archivo, int* contadorArray) {
	char **array = (char**)malloc(contar_lineas(archivo)*sizeof(char*));
	for(int x = 0; x < contar_lineas(archivo); x++) {
		array[x] = (char*) malloc(MAX_LINEA);
	}
	int flag = 0;
	int i = 0;
	int linea = 0;
	while(archivo[i] != '\0') {
		if (archivo[i] == '\n' && flag == 0) { //por si empieza con un \0
			strcpy(array[(*contadorArray)], "lineaVacia");
			(*contadorArray)++;
		} else if (archivo[i] == '\n' && archivo[i-1] == '\n') {
			strcpy(array[(*contadorArray)], "lineaVacia");
			(*contadorArray)++;
		} else if (archivo[i] != '\n' && archivo[i] != '\0') {
			linea++;
		} else if (archivo[i] == '\n') {
			strcpy(array[(*contadorArray)], string_substring(archivo, i-linea, linea));
			linea = 0;
			(*contadorArray)++;
		}
		i++;
		if (flag == 0) flag = 1;
	}
	return array;
}

void cargarArchivoAMemoriaTPI(int idProceso, char* path, char* archivo, int socketFD) {

}

void cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo, int socketFD) {
	int cantidadDeLineas = 0;
	char** arrayLineas = cargarArray(archivo, &cantidadDeLineas); //armo un array de punteros char
	int inicioDeLinea = lineasLibresConsecutivasMemoria(cantidadDeLineas);
	if (inicioDeLinea >= 0) {
		log_info(logger, "Hay espacio suficiente para cargar %s", path);
		//me fijo si ya tengo el proceso en lista y solo le agrego el archivo a su lista de segmentos
		//sino creo las estructuras
		if (existePID(idProceso)) {
			ProcesoArchivo* unProceso = obtenerProcesoId(idProceso);
			SegmentoArchivoSEG* nuevoSegmento = malloc(sizeof(SegmentoArchivoSEG));
			nuevoSegmento->idArchivo = malloc(strlen(path) + 1);
			strcpy(nuevoSegmento->idArchivo, path);
			nuevoSegmento->lineas = list_create();
			for (int x = 0; x < cantidadDeLineas; x++) {
				list_add(nuevoSegmento->lineas, arrayLineas[x]);
			}
			nuevoSegmento->inicio = inicioDeLinea;
			nuevoSegmento->fin = inicioDeLinea + cantidadDeLineas - 1; //-1 porque el inicioDeLinea puede ser 0
			list_add(unProceso->segmentos, nuevoSegmento);
			for(int x = 0; x < cantidadDeLineas; x++) {
				strcpy(memoria[inicioDeLinea], arrayLineas[x]);
				inicioDeLinea++;
			}
		} else {
			ProcesoArchivo* procesoArchivo = malloc(sizeof(procesoArchivo));
			procesoArchivo->idProceso = idProceso;
			procesoArchivo->segmentos = list_create();
			SegmentoArchivoSEG* unSegmentoArchivoSEG = malloc(sizeof(SegmentoArchivoSEG));
			unSegmentoArchivoSEG->idArchivo = malloc(strlen(path) + 1);
			strcpy(unSegmentoArchivoSEG->idArchivo, path);
			unSegmentoArchivoSEG->lineas = list_create();
			for (int x = 0; x < cantidadDeLineas; x++) {
				list_add(unSegmentoArchivoSEG->lineas, arrayLineas[x]); //ojo punteros
			}
			unSegmentoArchivoSEG->inicio = inicioDeLinea;
			unSegmentoArchivoSEG->fin = cantidadDeLineas - 1;
			list_add(procesoArchivo->segmentos, unSegmentoArchivoSEG);
			list_add(procesos, procesoArchivo);
			for(int x = 0; x < cantidadDeLineas; x++) {
				strcpy(memoria[inicioDeLinea], arrayLineas[x]);
				inicioDeLinea++;
			}
		}
		log_info(logger, "Archivo %s cargado correctamente", path);
		enviar_abrio_a_dam(socketFD, idProceso, path, archivo);
	} else {
		log_info(logger, "No hay espacio suficiente para cargar %s", path);
		Paquete paquete;
		paquete.header = cargar_header(0, ESPACIO_INSUFICIENTE_ABRIR, FM9);
		EnviarPaquete(socketFD, &paquete);
		log_header(logger, &paquete, "Mensaje de ERROR al Diego enviado");
	}
	free(arrayLineas);
}

void cargarArchivoAMemoriaSPA(int pid, char* path, char* archivo, int socketFD) {
	int cantidadDeLineas = 0;
	char** arrayLineas = cargarArray(archivo, &cantidadDeLineas); //array de punteros char
	//hay frames suficientes para guardar el archivo? (espacio en memoria)
	if(framesSuficientes(cantidadDeLineas)){
		log_info(logger, "Hay frames suficientes para cargar %s", path);
		//el proceso existe? cargo el archivo y se lo asigno; sino creo las estructuras
		if(obtenerProcesoId(pid)) {
			ProcesoArchivo* proceso = obtenerProcesoId(pid);
			SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
			segmento->idArchivo = malloc(strlen(path)+1);
			strcpy(segmento->idArchivo, path);
			segmento->paginas = list_create();
			segmento->cantidadPaginas = ceil((float) cantidadDeLineas/(float) lineasPorFrame); //si rompe, ver agregar -lm math.h o 'm' a libraries (eclipse)
			int lineas = 0;
			for(int x = 0; x < segmento->cantidadPaginas; x++) {
				int nroFrameLibre = primerFrameLibre();
				Pagina* pagina = malloc(sizeof(Pagina));
				Frame* frame = (Frame*) list_get(framesMemoria, nroFrameLibre);
				int inicioFrame = frame->inicio;
				while(lineas < cantidadDeLineas && inicioFrame <= frame->fin) {
					list_add(frame->lineas, arrayLineas[lineas]);
					strcpy(memoria[inicioFrame], arrayLineas[lineas]);
					inicioFrame++;
					lineas++;
				}
				frame->disponible = 0;
				pagina->numeroFrame = nroFrameLibre;
				list_add(segmento->paginas, pagina);
			}
			segmento->cantidadLineas = cantidadDeLineas;
			list_add(proceso->segmentos, segmento);
		} else {
			ProcesoArchivo* procesoNuevo = malloc(sizeof(ProcesoArchivo));
			procesoNuevo->idProceso = pid;
			procesoNuevo->segmentos = list_create();
			SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
			segmento->idArchivo = malloc(strlen(path)+1);
			strcpy(segmento->idArchivo, path);
			segmento->paginas = list_create();
			segmento->cantidadPaginas = ceil((float) (cantidadDeLineas)/(float) lineasPorFrame); //si rompe, ver agregar -lm math.h o 'm' a libraries (eclipse)
			int lineas = 0;
			for(int x = 0; x < segmento->cantidadPaginas; x++) {
				int nroFrameLibre = primerFrameLibre();
				Pagina* pagina = malloc(sizeof(Pagina));
				Frame* frame = (Frame*) list_get(framesMemoria, nroFrameLibre);
				int inicioFrame = frame->inicio;
				while(lineas < (cantidadDeLineas) && inicioFrame <= frame->fin) {
					list_add(frame->lineas, arrayLineas[lineas]);
					strcpy(memoria[inicioFrame], arrayLineas[lineas]);
					inicioFrame++;
					lineas++;
				}
				frame->disponible = 0;
				pagina->numeroFrame = nroFrameLibre;
				list_add(segmento->paginas, pagina);
			}
			segmento->cantidadLineas = (cantidadDeLineas);
			list_add(procesoNuevo->segmentos, segmento);
			list_add(procesos, procesoNuevo);
		}
		log_info(logger, "Archivo %s cargado correctamente", path);
		enviar_abrio_a_dam(socketFD, pid, path, archivo);
		log_info(logger, "ABRIR de %s realizado correctamente\nConfirmacion a DAM enviada", path);
	} else {
		log_info(logger, "No hay frames suficientes para cargar %s", path);
		Paquete paquete;
		paquete.header = cargar_header(0, ESPACIO_INSUFICIENTE_ABRIR, FM9);
		EnviarPaquete(socketFD, &paquete);
		log_header(logger, &paquete, "Mensaje de ERROR al Diego enviado");
	}
	free(arrayLineas);
}
////////////////////////////// ↑↑↑ PRIMITIVA ABRIR ↑↑↑ ////////////////////////////////////////////////

////////////////////////////// ↓↓↓  PRIMITIVA CLOSE ↓↓↓  //////////////////////////////////////////////
int liberarArchivoSPA(int pid, int seg, int socket) {
	Tipo tipoMensaje = 0;
	//traigo el segmento y las paginas que ocupa
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, seg);
	Pagina* paginaCero = list_get(segmento->paginas, 0);
	Frame* frameCero = list_get(framesMemoria, paginaCero->numeroFrame);
	log_info(logger, "Archivo a liberar: %s", segmento->idArchivo);
	//obtengo las paginas que posee el segmento del archivo a liberar
	log_info(logger, "La DF resuelta es: frame %d + offset %d -> linea %d", paginaCero->numeroFrame, 0, (frameCero->inicio + 0));
	log_info(logger, "El pid %d posee %d segmentos", pid, list_size(proceso->segmentos));
	for(int x = 0; x < segmento->cantidadPaginas; x++) {
		Pagina* pagina = list_get(segmento->paginas, x);
		Frame* frame = list_get(framesMemoria, pagina->numeroFrame);
		frame->disponible = 1;
		liberarMemoriaDesdeHasta(frame->inicio, frame->fin);
	}
	list_remove_and_destroy_element(proceso->segmentos, seg, LAMBDA(void _(SegmentoArchivoSPA* ss) {
					free(ss->idArchivo);
					list_destroy(ss->paginas);
					free(ss);}));
	log_info(logger, "Ahora el pid %d posee %d segmentos", pid, list_size(proceso->segmentos));
	tipoMensaje = CLOSE;
	return tipoMensaje;
}

int liberarArchivoSEG(int pid, int seg, int socket) {
	Tipo tipoMensaje = 0;
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, seg);
	log_info(logger, "Archivo a liberar: %s", segmento->idArchivo);
	log_info(logger, "La DF resuelta es: inicio de segmento %d + offset %d -> linea %d", segmento->inicio, 0, (segmento->inicio + 0));
	log_info(logger, "El pid %d posee %d segmentos", pid, list_size(proceso->segmentos));
	liberarMemoriaDesdeHasta(segmento->inicio, segmento->fin);
	list_remove_and_destroy_element(proceso->segmentos, seg,
			LAMBDA(void _(SegmentoArchivoSEG* ss) {
					free(ss->idArchivo);
					list_destroy(ss->lineas);
					free(ss);}));
	log_info(logger, "Ahora el pid %d posee %d segmentos", pid, list_size(proceso->segmentos));
	tipoMensaje = CLOSE;
	return tipoMensaje;
}
//////////////////////////////// ↑↑↑  PRIMITIVA CLOSE ↑↑↑  //////////////////////////////////////////

////////////////////////////// ↓↓↓ PRIMITIVA ASIGNAR ↓↓↓ ////////////////////////////////////////////
void asignarSPA(int pid, int seg, int pos, char* dato, int socketFD) {
//	int posReal = pos - 1; //dado que la linea 1 de un archivo, se persiste la posicion 0
	Paquete paquete;
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, seg);
	//esta dentro de su rango de memoria?
	int calculoDePagina = ceil((float) pos/(float) lineasPorFrame) -1;
	if(calculoDePagina < segmento->cantidadPaginas && pos < segmento->cantidadLineas) {
		//tengo el numero de pagina y la linea donde se ubica la posicion recibida
		Pagina* pag = list_get(segmento->paginas, calculoDePagina);
		int calculoDeLinea = pos - (calculoDePagina * lineasPorFrame);
		log_info(logger, "El dato a ASGINAR se encuentra en la pagina %d del segmento %d, linea %d de la misma", calculoDePagina, seg, calculoDeLinea);
		//busco el frame en memoria principal por linea de inicio
		Frame* frame = list_get(framesMemoria, pag->numeroFrame);
		list_replace(frame->lineas, calculoDeLinea, dato);
		strcpy(memoria[frame->inicio + calculoDeLinea], dato);
		log_info(logger, "La DF resuelta es frame %d + offset %d -> linea %d", pag->numeroFrame, pos, (frame->inicio + pos));
		paquete.header = cargar_header(0, ASIGNAR, FM9);
		EnviarPaquete(socketFD, &paquete);
		log_header(logger, &paquete, "Asignar de pid: %d del dato: %s en la linea: %d realizado con exito\nConfirmacion a DAM enviada", pid, dato, pos + 1);
	} else {
		log_info(logger, "La posicion: %d no pertenece al rango de memoria del pid: %d", pos + 1, pid);
		paquete.header = cargar_header(0, FALLO_DE_SEGMENTO_ASIGNAR, FM9);
		EnviarPaquete(socketFD, &paquete);
		log_header(logger, &paquete, "ASIGNAR Falló\nConfirmacion a DAM enviada");
	}
}

void asignarSEG(int pid, int seg, int pos, char* dato, int socketFD) {
//	int posReal = pos - 1; //dado que la linea 1 de un archivo, se persiste la posicion 0
	Paquete paquete;
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, seg);
	//la posicion solicitada le pertenece?
	if((segmento->inicio + pos) < segmento->fin) {
		list_replace(segmento->lineas, pos, dato);
		strcpy(memoria[segmento->inicio + pos], dato);
		log_info(logger, "El dato a ASGINAR se encuentra en el segmento %d, linea %d ", seg, pos);
		log_info(logger, "La DF resuelta es posicion de inicio de segmento %d + offset %d -> linea %d", segmento->inicio, pos, (segmento->inicio + pos));
		paquete.header = cargar_header(0, ASIGNAR, FM9);
		EnviarPaquete(socketFD, &paquete);
		log_header(logger, &paquete, "Asignar de pid: %d del dato: %s en la linea: %d realizado con exito", pid, dato, pos + 1);
	} else {
		log_info(logger, "La posicion: %d no pertenece al rango de memoria del pid: %d\n", pos + 1, pid);
		paquete.header = cargar_header(0, FALLO_DE_SEGMENTO_ASIGNAR, FM9);
		EnviarPaquete(socketFD, &paquete);
		log_header(logger, &paquete, "ASIGNAR Falló\nConfirmacion a DAM enviada");
	}
}
//////////////////////// ↑↑↑ PRIMITIVA ASIGNAR ↑↑↑ //////////////////////////////////////////

//////////////////////// ↓↓↓ PRIMITIVA FLUSH ↓↓↓ ////////////////////////////////////////////
void flushSEG(int seg, int pid, int socketFD) {
	Paquete* paquete = malloc(sizeof(Paquete));
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, seg);
	char* texto = malloc(MAX_LINEA * list_size(segmento->lineas));
	int posicionPtr = 0;
	int lineas = 0;
	for(int x = segmento->inicio; x < segmento->inicio + list_size(segmento->lineas); x++) {
		//transformo la lista de lineas en un char* concatenado por \n
		if(!strcmp(list_get(segmento->lineas, lineas), "lineaVacia")) {
			strcpy(texto, "");
		} else	strcpy(texto, list_get(segmento->lineas, lineas));
		size_t longitud = strlen(texto);
		*(texto + longitud) = '\n';
		*(texto + longitud + 1) = '\0';
		texto += longitud + 1;
		posicionPtr += longitud + 1;
		lineas++;
	}
	size_t longitud = strlen(texto);
	*(texto + longitud + 1) = '\0';
	texto = texto-posicionPtr;
	texto = realloc(texto, strlen(texto)+1);
	log_info(logger, "Texto a enviar a DAM:\n%s", texto);
	int text_size = 0;
	int path_size = 0;
	void* path_serial = string_serializar(segmento->idArchivo, &path_size);
	void* texto_serial = string_serializar(texto, &text_size);
	paquete->Payload = malloc(text_size+path_size);
	memcpy(paquete->Payload, path_serial, path_size);
	memcpy(paquete->Payload + path_size, texto_serial, text_size);
	paquete->header = cargar_header(path_size+text_size, GUARDAR_DATOS, FM9);
	EnviarPaquete(socketFD, paquete);
	log_header(logger, paquete, "FLUSH al archivo %s realizado con exito\nConfirmacion a DAM enviada", segmento->idArchivo);
	free(path_serial);
	free(text_size);
	free(paquete->Payload);
	free(paquete);
}

void flushSPA(int seg, int pid, int socketFD) {
	Paquete* paquete = malloc(sizeof(Paquete));
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, seg);
	char* texto = malloc(MAX_LINEA * (segmento->cantidadPaginas * lineasPorFrame));
	int lineas = 0;
	int posicionPtr = 0;
	for(int x = 0; x < segmento->cantidadPaginas; x++) {
		Pagina* pagina = list_get(segmento->paginas, x);
		Frame* frame = list_get(framesMemoria, pagina->numeroFrame);
		int inicioPagina = frame->inicio;
		int contador = 0;
		while(lineas < segmento->cantidadLineas && inicioPagina <= frame->fin) {
			if(!strcmp(list_get(frame->lineas, contador), "lineaVacia")) {
				strcpy(texto, "");
			} else	strcpy(texto, list_get(frame->lineas, contador));
			size_t longitud = strlen(texto);
			*(texto + longitud) = '\n';
			*(texto + longitud + 1) = '\0';
			texto += longitud + 1;
			posicionPtr += longitud + 1;
			contador++;
			lineas++;
			inicioPagina++;
		}
	}
	size_t longitud = strlen(texto);
	*(texto + longitud + 1) = '\0';
	texto = texto-posicionPtr;
	texto = realloc(texto, strlen(texto)+1);
	log_info(logger, "Texto a enviar a DAM:\n%s", texto);
	int text_size = 0;
	int path_size = 0;
	void* path_serial = string_serializar(segmento->idArchivo, &path_size);
	void* texto_serial = string_serializar(texto, &text_size);
	paquete->Payload = malloc(path_size + text_size);
	memcpy(paquete->Payload, path_serial, text_size);
	memcpy(paquete->Payload + text_size, texto_serial, path_size);
	paquete->header = cargar_header(path_size + text_size, GUARDAR_DATOS, FM9);
	EnviarPaquete(socketFD, paquete);
	log_header(logger, paquete, "FLUSH al archivo %s realizado con exito\nConfirmacion a DAM enviada", segmento->idArchivo);
	free(path_serial);
	free(texto_serial);
	free(paquete->Payload);
	free(paquete);
}
//////////////////////// ↑↑↑  PRIMITIVA FLUSH ↑↑↑  //////////////////////////////////////////

void accion(void *socket) {
	int socketFD = *(int *)socket;
	Paquete paquete;
	//while que recibe paquetes que envian a fm9, de qué socket y el paquete mismo
	//el socket del cliente conectado lo obtiene con la funcion servidorConcurrente en el main
	//El transfer size no puede ser menor al tamanio del header porque sino no se puede saber quien es el emisor
	while (RecibirDatos(&paquete.header , socketFD, TAMANIOHEADER) > 0) {
		log_header(logger, &paquete, "Interpreto mensaje del socket: %d", socketFD);
		switch (paquete.header.emisor) {
		case ELDIEGO: {
			if (paquete.header.tipoMensaje != ESHANDSHAKE && paquete.header.tamPayload > 0) {
				paquete.Payload = malloc(paquete.header.tamPayload);
				recibir_partes(socketFD, paquete.Payload, paquete.header.tamPayload);
			}
			manejar_paquetes_diego(&paquete, socketFD);
			break;
		}
		case CPU: {
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
//	if (paquete.Payload != NULL)
//		free(paquete.Payload);
	log_info(logger, "Hilo para conexion en socket %d terminado", socketFD);
	close(socketFD);
}

void manejar_paquetes_diego(Paquete* paquete, int socketFD) {
	int desplazamiento = 0;
	switch (paquete->header.tipoMensaje) {

		case ESHANDSHAKE: {
			socketElDiego = socketFD;
			paquete->Payload = malloc(paquete->header.tamPayload);
			RecibirDatos(paquete->Payload, socketFD, INTSIZE);
			memcpy(&transfer_size, paquete->Payload, INTSIZE);
			log_info(logger, "Transfersize recibido del Diego: %d", transfer_size);
			EnviarHandshakeELDIEGO(socketElDiego);
			log_info(logger, "Envío de HANDSHAKE al Diego exitoso");
			break;
		}

		case ABRIR: {
			int pid = 0;
			memcpy(&pid, paquete->Payload, INTSIZE);
			desplazamiento += INTSIZE;
			char *path = string_deserializar(paquete->Payload, &desplazamiento);
			char *archivo = string_deserializar(paquete->Payload, &desplazamiento);
			log_info(logger, "Paquete recibido con\npid: %d\npath: %s\narchivo: %s",
					pid, path, archivo);
			free(paquete->Payload);
			log_info(logger, "El archivo a cargar es: %s por el pid: %d", path, pid);
			sem_wait(&abrir);
			if(!strcmp(MODO, "SEG")) {
				cargarArchivoAMemoriaSEG(pid, path, archivo, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				cargarArchivoAMemoriaSPA(pid, path, archivo, socketFD);
			}
			sem_post(&abrir);
			break;
		}

		case FLUSH: {
			Posicion* posicion = deserializar_posicion(paquete->Payload, &desplazamiento);
			log_posicion(logger, posicion, "Recibo DL para flush");
			free(paquete->Payload);
			if(!strcmp(MODO, "SEG")) {
				flushSEG(posicion->segmento, posicion->pid, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				flushSPA(posicion->segmento, posicion->pid, socketFD);
			}
			break;
		}

		case FINALIZAR: {
			Posicion *posicion = deserializar_posicion(paquete->Payload, &desplazamiento);
			log_info(logger, "Recibo FINALIZAR para el pid %d", posicion->pid);
			logProcesoSEG(posicion->pid);
			Paquete* paqueteNuevo = malloc(sizeof(Paquete));
			paqueteNuevo->Payload = malloc(INTSIZE);
			paqueteNuevo->header = cargar_header(INTSIZE, SUCCESS, FM9);
			memcpy(paqueteNuevo->Payload, &posicion->pid, INTSIZE);
			ProcesoArchivo* proceso = obtenerProcesoId(posicion->pid);
			log_info(logger, "Se comienza a liberar los archivos del pid %d", posicion->pid);
			if(!strcmp(MODO, "SEG")) {
				//destruyo todos los segmentos del pid
				list_clean_and_destroy_elements(proceso->segmentos,
						LAMBDA(void _(SegmentoArchivoSEG* ss) {
							free(ss->idArchivo);
							liberarMemoriaDesdeHasta(ss->inicio, ss->fin);;
							list_destroy(ss->lineas);
							free(ss);}));
				log_debug(logger, "Se destruyeron todos los segmentos del pid %d", posicion->pid);
				//destruyo al pid de la lista de pid de la memoria
				list_remove_by_condition(procesos,
						LAMBDA(bool _(ProcesoArchivo* _proceso) {return _proceso->idProceso == posicion->pid;}));
				free(proceso);
				log_debug(logger, "Se destruyo el pid %d", posicion->pid);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				for(int x = 0; x < list_size(proceso->segmentos); x++) {
					SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, x);
					for(int y = 0; y < segmento->cantidadPaginas; y++) {
						Pagina* pagina = list_get(segmento->paginas, x);
						Frame* frame = list_get(framesMemoria, pagina->numeroFrame);
						frame->disponible = 1;
						liberarMemoriaDesdeHasta(frame->inicio, frame->fin);
					}
				}
				//destruyo todos los segmentos del pid
				list_destroy_and_destroy_elements(proceso->segmentos,
						LAMBDA(void _(SegmentoArchivoSPA* ss) {
							free(ss->idArchivo);
							list_destroy(ss->paginas);
							free(ss)
							;}));
				//destruyo al pid de la lista de pid de la memoria
				list_remove_and_destroy_by_condition(procesos,
						LAMBDA(bool _(ProcesoArchivo* proceso) {return proceso->idProceso == posicion->pid;}),
						free);
			}
			EnviarPaquete(socketFD, paqueteNuevo);
			log_header(logger, paqueteNuevo, "FINALIZAR exitoso del pid: %d y archivos asociados\nConfirmacion a CPU enviada", posicion->pid);
			free(paquete->Payload);
			free(paqueteNuevo->Payload);
			free(paqueteNuevo);
			break;
		}
	}
}

void manejar_paquetes_CPU(Paquete* paquete, int socketFD) {
	u_int32_t pid = 0;
	int tam_pid = sizeof(u_int32_t);
	int desplazamiento = 0;
	switch (paquete->header.tipoMensaje) {
		case ESHANDSHAKE: {
			EnviarHandshake(socketFD, FM9);
			log_info(logger, "Llegada de CPU con socket %d", socketFD);
			break;
		}

		case NUEVA_PRIMITIVA: {
			Posicion* posicion = deserializar_posicion(paquete->Payload, &desplazamiento);
			log_posicion(logger, posicion, "Recibo DL para buscar nueva primitiva");
			char *linea;
			if(!strcmp(MODO, "SEG")) {
				linea = lineaDeUnaPosicionSEG(posicion->pid, posicion->offset);
			} else if(!strcmp(MODO, "TPI")) {
				//
			} else if(!strcmp(MODO, "SPA")) {
				linea = lineaDeUnaPosicionSPA(posicion->pid, posicion->offset);
			}
			EnviarDatosTipo(socketFD, FM9, linea, strlen(linea)+1, LINEA_PEDIDA);
			log_info(logger, "Linea %s enviada correctamente al CPU %d", linea, socketFD);
			free(paquete->Payload);
			break;
		}

		case ASIGNAR: {
			Posicion *posicion = deserializar_posicion(paquete->Payload, &desplazamiento);
			char *dato = string_deserializar(paquete->Payload, &desplazamiento);
			log_posicion(logger, posicion, "Recibo DL para asignar el dato: %s", dato);
			if(!strcmp(MODO, "SEG")) {
				asignarSEG(posicion->pid, posicion->segmento, posicion->offset, dato, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				asignarSPA(posicion->pid, posicion->segmento, posicion->offset, dato, socketFD);
			}
			free(paquete->Payload);
			free(dato);
			break;
		}

		case CLOSE: {
			Tipo tipoMensaje = 0;
			Posicion *posicion = deserializar_posicion(paquete->Payload, &desplazamiento);
			log_posicion(logger, posicion, "Recibo DL para close");
			if(!strcmp(MODO, "SEG")) {
				tipoMensaje = liberarArchivoSEG(posicion->pid, posicion->segmento, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				tipoMensaje = liberarArchivoSPA(posicion->pid, posicion->segmento, socketFD);
			}
			Paquete respuesta;
			respuesta.header = cargar_header(0, tipoMensaje, FM9);
			EnviarPaquete(socketFD, &respuesta);
			log_header(logger, &respuesta, "CLOSE realizado correctamente\nConfirmacion a CPU enviada");
			free(paquete->Payload);
			break;
		}
	}
}

int contar_lineas (char *file) {
	int i, lineas = 0;
	for (i = 0; i < strlen(file); i++) 	{
		if(file[i] == '\n') lineas++;
	}
	return lineas;
}

int numeroDeSegmento(int pid, char* path) {
	int posicion = -1;
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	if(!strcmp(MODO, "SEG")) {
		for(int x = 0; list_size(proceso->segmentos); x++) {
			SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, x);
			if(!strcmp((segmento->idArchivo), path)) {
				posicion = x;
				break;
			}
		}
	} else if(!strcmp(MODO, "SPA")) {
		for(int x = 0; list_size(proceso->segmentos); x++) {
			SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, x);
			if(!strcmp((segmento->idArchivo), path)) {
				posicion = x;
				break;
			}
		}
	} else
		posicion = 0;

	return posicion;
}

int numeroDePagina(int pid, char* path) {
	if (!strcmp(MODO, "SPA") || !strcmp(MODO, "SEG"))
	return 0;
	else
		return -1;
}

void enviar_abrio_a_dam(int socketFD, u_int32_t pid, char *fid, char *file) {
	int desplazamiento = 0;
	int desplazamiento_archivo = 0;
	int tam_pid = sizeof(u_int32_t);

	ArchivoAbierto archivo;
	archivo.path = string_duplicate(fid);
	archivo.cantLineas = contar_lineas(file);
	archivo.segmento = numeroDeSegmento(pid, fid); // Aca iria el segmento donde esta cargado
	archivo.pagina = numeroDePagina(pid, fid); // Aca iria la pagina donde esta cargado
	void *archivo_serializado = DTB_serializar_archivo(&archivo, &desplazamiento_archivo);

	Paquete abrio;
	abrio.Payload = malloc(tam_pid + desplazamiento_archivo);
	memcpy(abrio.Payload, &pid, tam_pid);
	desplazamiento += tam_pid;
	memcpy(abrio.Payload + tam_pid, archivo_serializado, desplazamiento_archivo);
	desplazamiento += desplazamiento_archivo;

	abrio.header = cargar_header(desplazamiento, ABRIR, FM9);
	EnviarPaquete(socketFD, &abrio);
	log_header(logger, &abrio, "ABRIR de %s realizado correctamente\nConfirmacion a DAM enviada", fid);

	free(archivo_serializado);
	free(abrio.Payload);
}

void consola() {
	char* linea;
	while (true) {
		linea = (char*) readline(">> ");
		if(linea)
			add_history(linea);
		char** dump = (char**) string_split(linea, " ");

		if(!strcmp(dump[0], "dump")) {
			if(dump[1] == NULL) {
				log_info(logger, "Expected dump <id>");
				continue;
			}
			int idDump = atoi(dump[1]);
			if(!strcmp(MODO, "SEG"))
				logProcesoSEG(idDump);
			if(!strcmp(MODO, "SPA"))
				logProcesoSPA(idDump);
		}
		else if(!strcmp(dump[0], "memoria"))
			logMemoria();
		else if(!strcmp(dump[0], "fmemoria") && (!strcmp(MODO, "SPA") || (!strcmp(MODO, "TPI"))))
			logEstadoFrames();
		else printf("No se conoce el comando\n");
		free(linea);
		free(dump);
	}
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
	EnviarPaquete(socketFD, paquete);
	free(paquete->Payload);
	free(paquete);
}

void liberarMemoriaDesdeHasta(int nroLineaInicio, int nroLineaFin) {
	for(int x = nroLineaInicio; x <= nroLineaFin; x++) {
		strcpy(memoria[x], "NaN");
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
	for(int i = 0; i < lineasTotales; i++) {
		if(!strcmp(memoria[i], "NaN")) {
			contador++;
		} else {
			contador = 0;
		}
		if(contador == lineasAGuardar) {
			return (i-contador+1);
		}
	}
	return -1;
}

int framesSuficientes(int lineasAGuardar) {
	int lineasDisponibles = lineasPorFrame * (list_count_satisfying(framesMemoria, LAMBDA(bool _(Frame* frame) {return frame->disponible == 1;})));
	return lineasDisponibles >= lineasAGuardar;
}

int primerFrameLibre() {
	int posicion = -1;
	for(int x = 0; x < list_size(framesMemoria); x++) {
		Frame* frame = (Frame*) list_get(framesMemoria, x);
		if(frame->disponible) {
			posicion = x;
			break;
		}
	}
	return posicion;
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

////////////////////////////////////////// ↓↓↓ LOGGERS ↓↓↓  //////////////////////////////////////////
void logEstadoFrames() {
	log_info(logger, "Estado de Memoria Principal");
	for(int x = 0; x < list_size(framesMemoria); x++) {
		Frame* frame = list_get(framesMemoria, x);
		log_info(logger, "Frame numero: %d\ninicio: %d\nfin: %d\ndisponible: %d"
				, x, frame->inicio, frame->fin, frame->disponible);
		for (int y = 0; y < list_size(frame->lineas); y++) {
			log_info(logger, "elemento: %d con dato: %s", y, (char*) list_get(frame->lineas, y));
		}
	}
}

void logProcesoSEG(int pid) {
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	log_info(logger, "Info segmentos pid %d", pid);
	if(proceso && list_size(proceso->segmentos)) {
		for(int x = 0; x < list_size(proceso->segmentos); x++){
			SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, x);
			log_info(logger, "Segmento numero: %d\nPath: %s\nInicio en memoria real: %d\nFin en memoria real: %d\nCantidad de lineas: %d\nDatos en lineas:",
					x, segmento->idArchivo, segmento->inicio, segmento->fin, list_size(segmento->lineas));
			for(int y = 0; y < list_size(segmento->lineas); y++) {
				log_info(logger, "%s", list_get(segmento->lineas, y));
			}
		}
	} else log_info(logger, "El pid %d no posee archivos abiertos", pid);
}

void logProcesoSPA(int pid) {
	ProcesoArchivo* proceso = obtenerProcesoId(pid);
	log_info(logger, "Info segmentos pid %d", pid);
	if(list_size(proceso->segmentos)) {
		for(int x = 0; x < list_size(proceso->segmentos); x++){
			SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, x);
			log_info(logger, "Segmento numero: %d\nCantidad de paginas: %d\nCantidad de lineas: %d\nDatos en paginas:",
					x, segmento->cantidadPaginas, segmento->cantidadLineas);
			for(int y = 0; y < list_size(segmento->paginas); y++) {
				Pagina* pagina = list_get(segmento->paginas, y);
				log_info(logger, "Contenido de pagina %d: Frame numero %d",
						y, pagina->numeroFrame);
			}
		}
	} else log_info(logger, "El pid %d no posee archivos abiertos", pid);
}

void logMemoria() {
	for(int i = 0; i < lineasTotales; i++) {
		log_info(logger, "Posicion de memoria: linea %d, dato: %s", i, memoria[i]);
	}
	log_info(logger, "cantidad lineas libres de memoria: %d", cantidadLineasLibresMemoria());
}
////////////////////////////////////////// ↑↑↑ LOGGERS ↑↑↑  //////////////////////////////////////////

int main() {
	crearLogger();
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	inicializar(MODO);
	pthread_t hiloConsola; //un hilo para la consola
	pthread_create(&hiloConsola, NULL, (void*) consola, NULL);
	ServidorConcurrente(IP_FM9, PUERTO_FM9, FM9, &listaHilos, &end, accion);
	pthread_join(hiloConsola, NULL);
	return EXIT_SUCCESS;
}
