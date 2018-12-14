#include "fm9.h"
#include <commons/string.h>

char *MODO, *IP_FM9;
int PUERTO_FM9, TAMANIO, MAX_LINEA, TAM_PAGINA, socketElDiego, lineasTotales, framesTotales, lineasPorFrame, transfer_size;
char** memoria;

t_list* listaHilos;
t_list* framesMemoria;
t_list* procesos;
t_list* archivosPorProceso; //tabla global de pid-pathArchivo

sem_t abrir;

bool end;

t_log* logger;

////////////////////////////////////////// ↓↓↓  INICIALIZAR ↓↓↓  //////////////////////////////////////////
void crearLogger() {
	logger = log_create("FM9.log", "FM9", true, LOG_LEVEL_DEBUG);
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("./FM9.config");

	IP_FM9 = string_duplicate(config_get_string_value(arch, "IP_FM9"));
	log_warning(logger, "%s", IP_FM9);
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
	archivosPorProceso = list_create();
	lineasTotales = TAMANIO/MAX_LINEA;
	memoria = (char**) malloc(lineasTotales * sizeof(char*));
	for(int i = 0; i < lineasTotales; i++) {
		memoria[i] = (char*) malloc(MAX_LINEA);
		strcpy(memoria[i], "NaN");
	}
}

void inicializar(char* modo) {
	//“SEG” - “TPI” - “SPA”
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
	}
}
////////////////////////////////////////// ↑↑↑  INICIALIZAR ↑↑↑  //////////////////////////////////////////

///////////////////////////////////// ↓↓↓ AUX ↓↓↓ ////////////////////////////////////////
bool existePath(char* path) {
	return list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
}

bool existePID(int pid) {
	return list_any_satisfy(procesos, LAMBDA(bool _(ProcesoArchivo* proceso){return proceso->idProceso == pid;}));
}

int cantidadDeProcesosConElArchivoSEG(char* path) {
	int contador = 0;
	for(int x = 0; x < list_size(procesos); x++) {
		ProcesoArchivo* proceso = list_get(procesos, x);
		for(int y = 0; y < list_size(proceso->segmentos); y++) {
			SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, y);
			if(!strcmp(segmento->idArchivo, path)) contador++;
		}
	}
	return contador;
}

int cantidadDeProcesosConElArchivoSPA(char* path) {
	int contador = 0;
	for(int x = 0; x < list_size(procesos); x++) {
		ProcesoArchivo* proceso = list_get(procesos, x);
		for(int y = 0; y < list_size(proceso->segmentos); y++) {
			SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, y);
			if(!strcmp(segmento->idArchivo, path)) {
				contador++;
			}
		}
	}
	return contador;
}

bool elPidTieneElPath(int pid, char* path) {
	return list_any_satisfy(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path) && pp->idProceso == pid;}));
}

PidPath* obtenerPidPath(int pid, char* path) {
	return list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path) && pp->idProceso == pid;}));
}

ProcesoArchivo* obtenerProcesoId(int pid) {
	return list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
}

SegmentoArchivoSEG* obtenerSegmentoSEG(t_list* lista, char* path) {
	return list_find(lista, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
}

SegmentoArchivoSPA* obtenerSegmentoSPA(t_list* lista, char* path) {
	return list_find(lista, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
}
//////////////////////////////////// ↑↑↑ AUX ↑↑↑ /////////////////////////////////////////

///////////////////////////// ↓↓↓ LINEA DE UNA POSICION n ↓↓↓ //////////////////////////////
char* lineaDeUnaPosicionSPA(int pid, int pc) {
	int pcReal = pc-1;
	ProcesoArchivo* unProceso = obtenerProcesoId(pid);
	SegmentoArchivoSPA* segmento = list_get(unProceso->segmentos, 0);
	if(pcReal < segmento->cantidadLineas) {
		int calculoDePagina;
		if(pcReal) {
			calculoDePagina = ceil((float) pcReal/(float) lineasPorFrame) - 1; //el -1 es debido a que el index de la primer pagina en la lista es 0
		} else calculoDePagina = 0;
		int calculoDeLinea = pcReal - (lineasPorFrame * (calculoDePagina));
		Pagina* pagina = list_get(segmento->paginas, calculoDePagina);
		Frame* frame = list_find(framesMemoria, LAMBDA(bool _(Frame* f) {return pagina->inicio == f->inicio;}));
		printf("lineas por pagina: %d\n", lineasPorFrame);
		printf("nro pagina: %d\n", calculoDePagina);
		printf("inicio pagina: %d\n", pagina->inicio);
		printf("fin pagina: %d\n", pagina->fin);
		for(int x = 0; x < list_size(frame->lineas); x++) {
			printf("linea: %d contenido: %s\n", x,(char*) list_get(frame->lineas, x));
		}
		return ((char*) list_get(frame->lineas, calculoDeLinea));
	} else {
		//segmentation fault
		return NULL;
	}
}

char* lineaDeUnaPosicionSEG(int pid, int pc) {
	int pcReal = pc-1; //porque la linea 1 del archivo esta guardada en la posicion 0
	ProcesoArchivo* unProceso = obtenerProcesoId(pid);
	SegmentoArchivoSEG* unSegmento = list_get(unProceso->segmentos, 0);
	if(pcReal < unSegmento->fin) {
		return ((char*) list_get(unSegmento->lineas, pcReal));
	} else
		//segmentation fault
		return NULL;
}
//////////////////////// ↑↑↑ LINEA DE UNA POSICION n ↑↑↑ //////////////////////////////

//////////////////////// ↓↓↓ PRIMITIVA ABRIR ↓↓↓ //////////////////////////////////////
void cargarArchivoAMemoriaTPI(int idProceso, char* path, char* archivo, int socketFD) {

}

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
			flag = 1;
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
	}
	return array;
}

void cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo, int socketFD) {
	if(elPidTieneElPath(idProceso, path)) {
		log_info(logger, "el pid: %d ya tiene el archivo: %s\n", idProceso, path);
	//me fijo si el archivo ya lo posee un proceso y se lo asigno al nuevo. Lo COMPARTEN (VALIDAR EN CLOSE)
	} else if(list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return !strcmp(p->pathArchivo, path);}))) {
		//me traigo el proceso que contiene el archivo pedido
		PidPath* pp = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return !strcmp(p->pathArchivo, path);}));
		ProcesoArchivo* procesoConElArchivo = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* proceso) {return proceso->idProceso == pp->idProceso;}));
		SegmentoArchivoSEG* segmentoConElArchivo = obtenerSegmentoSEG(procesoConElArchivo->segmentos, path);
		//si el proceso ya existe se lo agrego, sino creo todas las estructuras
		if(obtenerProcesoId(idProceso)) {
			ProcesoArchivo* proceso = obtenerProcesoId(idProceso);
			SegmentoArchivoSEG* segmentoNuevo = malloc(sizeof(SegmentoArchivoSEG));
			segmentoNuevo->idArchivo = malloc(strlen(path)+1);
			strcpy(segmentoNuevo->idArchivo, path);
			segmentoNuevo->lineas = list_create();
			//segmentoNuevo->lineas = list_duplicate(segmentoConElArchivo->lineas); //duda, duplica los punteros?
			//la opcion manual: (?
			for(int x = 0; x < list_size(segmentoConElArchivo->lineas); x++) {
				strcpy(list_get(segmentoNuevo->lineas, x), list_get(segmentoConElArchivo->lineas, x));
			}
			segmentoNuevo->inicio = segmentoConElArchivo->inicio;
			segmentoNuevo->fin = segmentoConElArchivo->fin;
			list_add(proceso->segmentos, segmentoNuevo);
		} else {
			ProcesoArchivo* procesoArchivo = malloc(sizeof(procesoArchivo));
			procesoArchivo->idProceso = idProceso;
			procesoArchivo->segmentos = list_create();
			SegmentoArchivoSEG* unSegmentoArchivoSEG = malloc(sizeof(SegmentoArchivoSEG));
			unSegmentoArchivoSEG->idArchivo = malloc(strlen(path) + 1);
			strcpy(unSegmentoArchivoSEG->idArchivo, path);
			unSegmentoArchivoSEG->lineas = list_create();
			//unSegmentoArchivoSEG->lineas = list_duplicate(segmentoConElArchivo->lineas); duda
			//la opcion manual: (?
			for(int x = 0; x < list_size(segmentoConElArchivo->lineas); x++) {
				char* dato = malloc(strlen(list_get(segmentoConElArchivo->lineas, x)) + 1);
				strcpy(dato, list_get(segmentoConElArchivo->lineas, x));
				list_add(unSegmentoArchivoSEG->lineas, dato);
			}
			unSegmentoArchivoSEG->inicio = segmentoConElArchivo->inicio;
			unSegmentoArchivoSEG->fin = segmentoConElArchivo->fin;
			list_add(procesoArchivo->segmentos, unSegmentoArchivoSEG);
			list_add(procesos, procesoArchivo);
		}
		agregarArchivoYProcesoALista(idProceso, path);
		log_info(logger, "El archivo ya cargado fue agregado a los archivos abiertos del proceso: %d\n", idProceso);
		enviar_abrio_a_dam(socketFD, idProceso, path, archivo);
	} else {
//		char** arrayLineas = (char**) string_split(archivo, "\n"); //deja un elemento con NULL al final
		int cantidadDeLineas = 0;
		char** arrayLineas = cargarArray(archivo, &cantidadDeLineas);
//		int cantidadDeLineas = contarElementosArray(arrayLineas);
		log_info(logger, "El archivo a cargar es: %s\n", path);
		int inicioDeLinea = lineasLibresConsecutivasMemoria(cantidadDeLineas);
		if (inicioDeLinea >= 0) {
			//me fijo si ya tengo el proceso en lista y solo le agrego el archivo a su lista de segmentos
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
				nuevoSegmento->fin = inicioDeLinea + cantidadDeLineas - 1;
				list_add(unProceso->segmentos, nuevoSegmento);
				for(int x = 0; x < cantidadDeLineas; x++) {
					strcpy(memoria[inicioDeLinea], arrayLineas[x]);
					inicioDeLinea++;
				}
				log_info(logger, "Archivo: %s cargado correctamente en el pid: %d\n", path, idProceso);
			} else {
				ProcesoArchivo* procesoArchivo = malloc(sizeof(procesoArchivo));
				procesoArchivo->idProceso = idProceso;
				procesoArchivo->segmentos = list_create();
				SegmentoArchivoSEG* unSegmentoArchivoSEG = malloc(sizeof(SegmentoArchivoSEG));
				unSegmentoArchivoSEG->idArchivo = malloc(strlen(path) + 1);
				strcpy(unSegmentoArchivoSEG->idArchivo, path);
				unSegmentoArchivoSEG->lineas = list_create();
				for (int x = 0; x < cantidadDeLineas; x++) {
					list_add(unSegmentoArchivoSEG->lineas, arrayLineas[x]);
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
			log_info(logger, "Archivo: %s cargado correctamente en el pid: %d\n", path, idProceso);
			agregarArchivoYProcesoALista(idProceso, path);
			log_info(logger, "---------------------------El archivo a cargar es: %s\n", path);
			enviar_abrio_a_dam(socketFD, idProceso, path, archivo);
		} else {
			log_info(logger, "No hay espacio suficiente para cargar el segmento\n");
			Paquete paquete;
			paquete.header = cargar_header(0, ESPACIO_INSUFICIENTE_ABRIR, FM9);
			EnviarPaquete(socketFD, &paquete);
		}
		free(arrayLineas);
	}
}

//optimizar repeticion de codigo
void cargarArchivoAMemoriaSPA(int pid, char* path, char* archivo, int socketFD) {
	//ya tiene el archivo el proceso?
	if(elPidTieneElPath(pid, path)) {
		log_info(logger, "El pid: %d ya tiene el archivo: %s\n", pid, path);
	}
	//el archivo ya existe?
	else if(existePath(path)) {
		//obtengo el segmento y sus paginas que contienen al archivo (Se COMPARTE, ver CLOSE)
		PidPath* pidPathConElPath = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* procesoConElPath = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pidPathConElPath->idProceso;}));
		SegmentoArchivoSPA* segmentoConElPath = obtenerSegmentoSPA(procesoConElPath->segmentos, path);
		//el proceso es nuevo o ya existe?
		if(obtenerProcesoId(pid)) {
			//sólo le agrego el archivo
			ProcesoArchivo* proceso = obtenerProcesoId(pid);
			SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
			segmento->idArchivo = malloc(strlen(path)+1);
			strcpy(segmento->idArchivo, path);
			segmento->paginas = list_create();
			//segmento->paginas = list_duplicate(segmentoConElPath->paginas); //voy a tener el mismo ptr de los 2 lados? duda
			//forma manual:
			for(int x = 0; x < segmentoConElPath->cantidadPaginas; x++) {
				Pagina* paginaConElPath = list_get(segmentoConElPath->paginas, x);
				Pagina* pagina = malloc(sizeof(Pagina));
				pagina->inicio = paginaConElPath->inicio;
				pagina->fin = paginaConElPath->fin;
				list_add(segmento->paginas, pagina);
			}
			segmento->cantidadPaginas = segmentoConElPath->cantidadPaginas;
			segmento->cantidadLineas = segmentoConElPath->cantidadLineas;
			list_add(proceso->segmentos, segmento);
		} else {
			//cargo nueva estructura de proceso y le agrego el archivo
			ProcesoArchivo* procesoNuevo = malloc(sizeof(ProcesoArchivo));
			procesoNuevo->idProceso = pid;
			procesoNuevo->segmentos = list_create();
			SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
			segmento->idArchivo = malloc(strlen(path)+1);
			strcpy(segmento->idArchivo, path);
			segmento->paginas = list_create();
			//segmento->paginas = list_duplicate(segmentoConElPath->paginas); // duda
			//forma manual
			for(int x = 0; x < segmentoConElPath->cantidadPaginas; x++) {
				Pagina* paginaConElPath = list_get(segmentoConElPath->paginas, x);
				Pagina* pagina = malloc(sizeof(Pagina));
				pagina->inicio = paginaConElPath->inicio;
				pagina->fin = paginaConElPath->fin;
				list_add(segmento->paginas, pagina);
			}
			segmento->cantidadPaginas = segmentoConElPath->cantidadPaginas;
			segmento->cantidadLineas = segmentoConElPath->cantidadLineas;
			list_add(procesoNuevo->segmentos, segmento);
			list_add(procesos, procesoNuevo);
		}
		agregarArchivoYProcesoALista(pid, path);
		enviar_abrio_a_dam(socketFD, pid, path, archivo);
		log_info(logger, "Archivo: %s de pid: %d, cargado con éxito\n", path, pid);
	} else {
		//el archivo no existe? lo cargo
//		char** arrayLineas = (char**) string_split(archivo, "\n"); //deja un elemento con NULL al final
//		int cantidadDeLineas = contarElementosArray(arrayLineas);
		int cantidadDeLineas = 0;
		char** arrayLineas = cargarArray(archivo, &cantidadDeLineas);
		//el proceso existe?
		if(obtenerProcesoId(pid)) {
			//el proceso existe, cargo el archivo y se lo asigno
			//hay frames libres para cargar el archivo? (espacio en memoria)
			if(framesSuficientes(cantidadDeLineas)) {
				ProcesoArchivo* proceso = obtenerProcesoId(pid);
				SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
				segmento->idArchivo = malloc(strlen(path)+1);
				strcpy(segmento->idArchivo, path);
				segmento->paginas = list_create();
				segmento->cantidadPaginas = ceil((float) cantidadDeLineas/(float) lineasPorFrame); //si rompe, ver agregar -lm math.h o 'm' a libraries (eclipse)
				int lineas = 0;
				for(int x = 0; x < segmento->cantidadPaginas; x++) {
					Pagina* pagina = malloc(sizeof(Pagina));
					Frame* frame = (Frame*) list_get(framesMemoria, primerFrameLibre());
					int inicioFrame = frame->inicio;
					while(lineas < cantidadDeLineas && inicioFrame <= frame->fin) {
						list_add(frame->lineas, arrayLineas[lineas]);
						strcpy(memoria[inicioFrame], arrayLineas[lineas]);
						inicioFrame++;
						lineas++;
					}
					frame->disponible = 0;
					pagina->inicio = frame->inicio;
					pagina->fin = frame->fin;
					list_add(segmento->paginas, pagina);
				}
				segmento->cantidadLineas = cantidadDeLineas;
				list_add(proceso->segmentos, segmento);
				agregarArchivoYProcesoALista(pid, path);
				log_info(logger, "Archivo: %s de pid: %d, cargado con éxito\n", path, pid);
			} else {
				log_info(logger, "No hay frames libres para cargar el archivo: %s de pid: %d\n", path, pid);
				Paquete paquete;
				paquete.header = cargar_header(0, ESPACIO_INSUFICIENTE_ABRIR, FM9);
				EnviarPaquete(socketFD, &paquete);
			}
		} else {
			//el proceso no existe, lo creo
			//hay frames libres para cargar el archivo? (espacio en memoria)
			if(framesSuficientes(cantidadDeLineas)) {
				ProcesoArchivo* procesoNuevo = malloc(sizeof(ProcesoArchivo));
				procesoNuevo->idProceso = pid;
				procesoNuevo->segmentos = list_create();
				SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
				segmento->idArchivo = malloc(strlen(path)+1);
				strcpy(segmento->idArchivo, path);
				segmento->paginas = list_create();
				segmento->cantidadPaginas = ceil((float) cantidadDeLineas/(float) lineasPorFrame); //si rompe, ver agregar -lm math.h o 'm' a libraries (eclipse)
				int lineas = 0;
				for(int x = 0; x < segmento->cantidadPaginas; x++) {
					Pagina* pagina = malloc(sizeof(Pagina));
					Frame* frame = (Frame*) list_get(framesMemoria, primerFrameLibre());
					int inicioFrame = frame->inicio;
					while(lineas < cantidadDeLineas && inicioFrame <= frame->fin) {
						list_add(frame->lineas, arrayLineas[lineas]);
						strcpy(memoria[inicioFrame], arrayLineas[lineas]);
						inicioFrame++;
						lineas++;
					}
					frame->disponible = 0;
					pagina->inicio = frame->inicio;
					pagina->fin = frame->fin;
					list_add(segmento->paginas, pagina);
				}
				segmento->cantidadLineas = cantidadDeLineas;
				list_add(procesoNuevo->segmentos, segmento);
				list_add(procesos, procesoNuevo);
				agregarArchivoYProcesoALista(pid, path);
				log_info(logger, "Archivo: %s de pid: %d, cargado con éxito\n", path, pid);
				enviar_abrio_a_dam(socketFD, pid, path, archivo);
			} else {
				log_info(logger, "No hay frames libres para cargar el archivo: %s de pid: %d\n", path, pid);
				Paquete paquete;
				paquete.header = cargar_header(0, ESPACIO_INSUFICIENTE_ABRIR, FM9);
				EnviarPaquete(socketFD, &paquete);
			}
		}
		free(arrayLineas);
	}
}
////////////////////////////// ↑↑↑ PRIMITIVA ABRIR ↑↑↑ ////////////////////////////////////////////////

////////////////////////////// ↓↓↓  PRIMITIVA CLOSE ↓↓↓  //////////////////////////////////////////////
void liberarArchivoSPA(int pid, char* path, int socket) {
	//le pertenece el path al pid?
	Paquete paquete;
	if (elPidTieneElPath(pid, path)) {
		//traigo el segmento y las paginas que ocupa
		ProcesoArchivo* proceso = obtenerProcesoId(pid);
		SegmentoArchivoSPA* segmento = obtenerSegmentoSPA(proceso->segmentos, path);
		//hay mas de un proceso con el mismo archivo?
		if (cantidadDeProcesosConElArchivoSPA(path) < 2) {
			//es el único -> lo borro de memoria
			//obtengo las paginas que posee el segmento del archivo a liberar
			for(int x = 0; x < segmento->cantidadPaginas; x++) {
				Pagina* pagina = list_get(segmento->paginas, x);
				//busco el frame en memoria principal por linea de inicio
				Frame* frame = list_find(framesMemoria, LAMBDA(bool _(Frame* f) {return f->inicio == pagina->inicio;}));
				frame->disponible = 1;
				list_clean_and_destroy_elements(frame->lineas, LAMBDA(void _(char* linea) {free(linea);}));
				liberarMemoriaDesdeHasta(frame->inicio, frame->fin);
			}
		}
		list_remove_and_destroy_by_condition(proceso->segmentos,
				LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}),
				LAMBDA(void _(SegmentoArchivoSPA* ss) {free(ss->idArchivo); list_destroy(ss->paginas); free(ss);}));
		quitarArchivoYProcesoDeTabla(pid, path);
		log_info(logger, "Archivo %s liberado correctamente\n", path);
		paquete.header = cargar_header(0, CLOSE, FM9);
		EnviarPaquete(socket, &paquete);
	} else {
		log_info(logger, "el archivo %s no se encuentra abierto por el pid %d\n", path, pid);
		paquete.header = cargar_header(0, FALLO_DE_SEGMENTO_CLOSE, FM9);
		EnviarPaquete(socket, &paquete);
	}
}

void liberarArchivoSEG(int pid, char* path, int socket) {
	//le pertenece el path al pid?
	Paquete paquete;
	if (elPidTieneElPath(pid, path)) {
		ProcesoArchivo* proceso = obtenerProcesoId(pid);
		SegmentoArchivoSEG* segmento = obtenerSegmentoSEG(proceso->segmentos, path);
		//si el archivo lo tiene abierto un unico pid, lo limpio de memoria
		if (cantidadDeProcesosConElArchivoSEG(path)<2)
			liberarMemoriaDesdeHasta(segmento->inicio, segmento->fin);
		list_remove_and_destroy_by_condition(proceso->segmentos,
				LAMBDA(bool _(SegmentoArchivoSEG* s) {return !strcmp(s->idArchivo, path);}),
				LAMBDA(void _(SegmentoArchivoSEG* ss) {
			free(ss->idArchivo);
			list_destroy_and_destroy_elements(ss->lineas, LAMBDA(void _(char* linea) {free(linea);}));
			free(ss);}));
		log_info(logger, "Archivo %s liberado correctamente\n", path);
		paquete.header = cargar_header(0, CLOSE, FM9);
		EnviarPaquete(socket, &paquete);
		quitarArchivoYProcesoDeTabla(pid, path);
	} else {
		log_info(logger, "el archivo %s no se encuentra abierto por el pid %d\n", path, pid);
		paquete.header = cargar_header(0, FALLO_DE_SEGMENTO_CLOSE, FM9);
		EnviarPaquete(socket, &paquete);
	}
}
//////////////////////////////// ↑↑↑  PRIMITIVA CLOSE ↑↑↑  //////////////////////////////////////////

////////////////////////////// ↓↓↓ PRIMITIVA ASIGNAR ↓↓↓ ////////////////////////////////////////////
void asignarSPA(int pid, char* path, int pos, char* dato, int socketFD) {
	int posReal = pos - 1; //dado que la linea 1 de un archivo, se persiste la posicion 0
	//posee el archivo?
	if(elPidTieneElPath(pid, path)) {
		Paquete paquete;
		ProcesoArchivo* proceso = obtenerProcesoId(pid);
		SegmentoArchivoSPA* segmento = obtenerSegmentoSPA(proceso->segmentos, path);
		//esta dentro de su rango de memoria?
		int calculoDePagina = ceil((float) posReal/(float) lineasPorFrame) -1;
		if(calculoDePagina < segmento->cantidadPaginas && pos <= segmento->cantidadLineas) {
			//tengo el numero de pagina y la linea donde se ubica la posicion recibida
			Pagina* pag = list_get(segmento->paginas, calculoDePagina);
			int calculoDeLinea = posReal - (calculoDePagina * lineasPorFrame);
			//busco el frame en memoria principal por linea de inicio
			Frame* frame = list_find(framesMemoria, LAMBDA(bool _(Frame* f) {return f->inicio == pag->inicio;}));
			list_replace(frame->lineas, calculoDeLinea, dato);
			strcpy(memoria[frame->inicio + calculoDeLinea], dato);
			log_info(logger, "Asignar de pid: %d del dato: %s en la posicion: %d realizado con exito", pid, dato, pos);
			paquete.header = cargar_header(0, ASIGNAR, FM9);
		} else {
			log_info(logger, "la posicion: %d no pertenece al rango de memoria del pid: %d\n", pos, pid);
			paquete.header = cargar_header(0, FALLO_DE_SEGMENTO_ASIGNAR, FM9);
		}
		EnviarPaquete(socketFD, &paquete);
	} else {
		log_info(logger, "el pid: %d no posee el archivo: %s\n", pid, path);
		EnviarDatosTipo(socketFD, FM9, NULL, 0, SUCCESS);
	}
}

void asignarSEG(int pid, char* path, int pos, char* dato, int socketFD) {
	int posReal = pos - 1; //dado que la linea 1 de un archivo, se persiste la posicion 0
	//posee el archivo?
	if(elPidTieneElPath(pid, path)) {
		Paquete paquete;
		//la posicion solicitada le pertenece?
		ProcesoArchivo* proceso = obtenerProcesoId(pid);
		SegmentoArchivoSEG* segmento = obtenerSegmentoSEG(proceso->segmentos, path);
		if((segmento->inicio + posReal) < segmento->fin) {
			list_replace(segmento->lineas, posReal, dato);
			strcpy(memoria[segmento->inicio + posReal], dato);
			log_info(logger, "Asignar de pid: %d del dato: %s en la posicion: %d realizado con exito", pid, dato, pos);
			paquete.header = cargar_header(0, ASIGNAR, FM9);
		} else {
			log_info(logger, "la posicion: %d no pertenece al rango de memoria del pid: %d\n", pos, pid);
			paquete.header = cargar_header(0, FALLO_DE_SEGMENTO_ASIGNAR, FM9);
		}
		EnviarPaquete(socketFD, &paquete);
	} else {
		log_info(logger, "el pid: %d no posee el archivo: %s\n", pid, path);
		EnviarDatosTipo(socketFD, FM9, NULL, 0, SUCCESS);
	}
}
//////////////////////// ↑↑↑ PRIMITIVA ASIGNAR ↑↑↑ //////////////////////////////////////////

//////////////////////// ↓↓↓ PRIMITIVA FLUSH ↓↓↓ ////////////////////////////////////////////
void flushSEG(char* path, int socketFD) {
	//archivo abierto?
	if(existePath(path)) {
		PidPath* pp = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* proceso = obtenerProcesoId(pp->idProceso);
		SegmentoArchivoSEG* segmento = obtenerSegmentoSEG(proceso->segmentos, path);
		char* texto = malloc(MAX_LINEA * list_size(segmento->lineas));
		int posicionPtr = 0;
		int lineas = 0;
		for(int x = segmento->inicio; x < segmento->inicio + list_size(segmento->lineas); x++) {
			//transformo la lista de lineas en un char* concatenado por \n
			strcpy(texto, list_get(segmento->lineas, lineas));
			size_t longitud = strlen(texto);
			*(texto + longitud) = '\n';
			*(texto + longitud + 1) = '\0';
			texto += longitud + 1;
			posicionPtr += longitud + 1;
			lineas++;
		}
		size_t longitud = strlen(texto);
		*(texto + longitud) = '\n';
		*(texto + longitud + 1) = '\n';
		*(texto + longitud + 2) = '\0';
		texto = texto-posicionPtr;
		texto = realloc(texto, strlen(texto)+1);
		printf("%s", texto);
		//la variable texto es la que se debe enviar a DAM,
		//contiene las linas unidas por \n y \n\n\0 al final
	} else {
		printf("el archivo no se encuentra abierto\n");
	}
}

void flushSPA(char* path, int socketFD) {
	//archivo abierto?
	if(existePath(path)) {
		PidPath* pp = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* proceso = obtenerProcesoId(pp->idProceso);
		SegmentoArchivoSPA* segmento = obtenerSegmentoSPA(proceso->segmentos, path);
		char* texto = malloc(MAX_LINEA * (segmento->cantidadPaginas * lineasPorFrame));
		int lineas = 0;
		int posicionPtr = 0;
		for(int x = 0; x < segmento->cantidadPaginas; x++) {
			Pagina* pagina = list_get(segmento->paginas, x);
			Frame* frame = list_find(framesMemoria, LAMBDA(bool _(Frame* f) {return pagina->inicio == f->inicio;}));
			int inicioPagina = pagina->inicio;
			int contador = 0;
			while(lineas < segmento->cantidadLineas && inicioPagina <= pagina->fin) {
				strcpy(texto, list_get(frame->lineas, contador));
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
		*(texto + longitud) = '\n';
		*(texto + longitud + 1) = '\n';
		*(texto + longitud + 2) = '\0';
		texto = texto-posicionPtr;
		texto = realloc(texto, strlen(texto)+1);
		printf("%s", texto);
		//la variable texto es la que se debe enviar a DAM,
		//contiene las linas unidas por \n y \n\n\0 al final
	} else {
		printf("el archivo no se encuentra abierto\n");
	}
}
//////////////////////// ↑↑↑  PRIMITIVA FLUSH ↑↑↑  //////////////////////////////////////////

void accion(void *socket) {
	int socketFD = *(int *)socket;
	Paquete paquete;
	//while que recibe paquetes que envian a safa, de qué socket y el paquete mismo
	//el socket del cliente conectado lo obtiene con la funcion servidorConcurrente en el main
	//El transfer size no puede ser menor al tamanio del header porque sino no se puede saber quien es el emisor
	//while (RecibirPaqueteServidorSafa(socketFD, SAFA, &paquete) > 0)
	while (RecibirDatos(&paquete.header , socketFD, TAMANIOHEADER) > 0) {
		log_debug(logger, "Recibi paquete en socket %d", socketFD);
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
	if (paquete.Payload != NULL)
		free(paquete.Payload);
	close(socketFD);
	log_debug(logger, "Hilo para conexion en socket %d terminado", socketFD);
}

void manejar_paquetes_diego(Paquete* paquete, int socketFD) {
	log_debug(logger, "Interpreto mensajes del diego");
	u_int32_t pid = 0;
	int tam_pid = sizeof(u_int32_t);
	switch (paquete->header.tipoMensaje) {

		case ESHANDSHAKE: {
			socketElDiego = socketFD;
			paquete->Payload = malloc(paquete->header.tamPayload);
			RecibirDatos(paquete->Payload, socketFD, INTSIZE);
			memcpy(&transfer_size, paquete->Payload, INTSIZE);
			log_info(logger, "transfersize del diego %d\n", transfer_size);
			EnviarHandshakeELDIEGO(socketElDiego);
			break;
		}

		case ABRIR: {
			sem_wait(&abrir);
			memcpy(&pid, paquete->Payload, tam_pid);
			int tam_desplazado = tam_pid;
			log_debug(logger, "pid: %d", pid);
			char *path = string_deserializar(paquete->Payload, &tam_desplazado);
			log_debug(logger, "path: %s", path);
			char *archivo = string_deserializar(paquete->Payload, &tam_desplazado);
			log_debug(logger, "file:\n%s", archivo);
			free(paquete->Payload);

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
			//el payload viene con el path
			char* path = malloc(strlen(paquete->Payload)+ 1);
			strcpy(path, paquete->Payload);
			free(paquete);
			lockearArchivo(path);
			if(!strcmp(MODO, "SEG")) {
				flushSEG(path, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				flushSPA(path, socketFD);
			}
			deslockearArchivo(path);
			break;
		}
	}
}

void manejar_paquetes_CPU(Paquete* paquete, int socketFD) {
	log_debug(logger, "Interpreto mensajes de cpu");
	u_int32_t pid = 0;
	int tam_pid = sizeof(u_int32_t);
	switch (paquete->header.tipoMensaje) {
		case ESHANDSHAKE: {
			EnviarHandshake(socketFD, FM9);
			log_info(logger, "llegada cpu con id %d", socketFD);
			break;
		}

		case NUEVA_PRIMITIVA: {
			//el payload debe ser: int(pid)+int(pc)
			int pid, pc;
			memcpy(&pid, paquete->Payload, sizeof(pid));
			paquete->Payload += sizeof(pid);
			memcpy(&pc, paquete->Payload, sizeof(pc));
			paquete->Payload += sizeof(pc);
			int d = 0;
			ArchivoAbierto* archivo = DTB_leer_struct_archivo(paquete->Payload, &d);
			paquete->Payload += d;
			char* linea = malloc(MAX_LINEA);
			if(!strcmp(MODO, "SEG")) {
				if(lineaDeUnaPosicionSEG(pid, pc) != NULL) {
					linea = lineaDeUnaPosicionSEG(pid, pc);
					linea = realloc(linea, strlen(linea)+1);
					log_info(logger, "La direccion logica recibida por pid: %d es,\n segmento: %d + offset: %d\n", pid, archivo->segmento, pc);
					EnviarDatosTipo(socketFD, CPU, linea, strlen(linea)+1, LINEA_PEDIDA);
				} else
					//segmentation fault
					log_info(logger, "segmentation fault (?? no deberia llegar");
					EnviarDatosTipo(socketFD, CPU, NULL, 0, ERROR);
			} else if(!strcmp(MODO, "TPI")) {
				//
			} else if(!strcmp(MODO, "SPA")) {
				if(lineaDeUnaPosicionSPA(pid, pc) != NULL) {
					linea = lineaDeUnaPosicionSPA(pid, pc);
					linea = realloc(linea, strlen(linea)+1);
					log_info(logger, "La direccion logica recibida por pid: %d es,\n segmento: %d + pagina: %d + offset: %d\n", pid, archivo->segmento, archivo->pagina, pc);
					EnviarDatosTipo(socketFD, CPU, linea, strlen(linea)+1, LINEA_PEDIDA);
				} else
					//segmentation fault
					log_info(logger, "segmentation fault (?? no deberia llegar");
					EnviarDatosTipo(socketFD, CPU, NULL, 0, ERROR);
			}
			free(linea);
			paquete->Payload -= paquete->header.tamPayload;
			free(paquete->Payload);
			break;
		}

		case ASIGNAR: {
			//el payload debe ser int+char\0+int+char\0
			u_int32_t pid, pos, d = 0;
			memcpy(&pid, paquete->Payload, INTSIZE);
			paquete->Payload += INTSIZE;
			memcpy(&pos, paquete->Payload, INTSIZE);
			paquete->Payload += INTSIZE;
			ArchivoAbierto* archivo = DTB_leer_struct_archivo(paquete->Payload, &d);
			char* dato = string_deserializar(paquete->Payload, &d);
			paquete->Payload += d;
			lockearArchivo(archivo->path);
			if(!strcmp(MODO, "SEG")) {
				asignarSEG(pid, archivo->path, pos, dato, socketFD);
				log_info(logger, "Llegada de DL: segmento %d + offset %d", archivo->segmento, pos);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				asignarSPA(pid, archivo->path, pos, dato, socketFD);
				log_info(logger, "Llegada de DL: segmento %d + pagina %d + offset %d", archivo->segmento, archivo->pagina, pos);
			}
			deslockearArchivo(archivo->path);
			paquete->Payload -= paquete->header.tamPayload;
			free(paquete->Payload);
			free(paquete);
			liberar_archivo_abierto(archivo);
			free(dato);
			break;
		}

		case CLOSE: {
			//el payload debe ser int+char\0
			u_int32_t pid = 0;
			int desplazamiento = 0;
			memcpy(&pid, paquete->Payload, INTSIZE);
			desplazamiento += INTSIZE;
			ArchivoAbierto *archivo = DTB_leer_struct_archivo(paquete->Payload, &desplazamiento);
			lockearArchivo(archivo->path);
			if(!strcmp(MODO, "SEG")) {
				liberarArchivoSEG(pid, archivo->path, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				liberarArchivoSPA(pid, archivo->path, socketFD);
			}
			deslockearArchivo(archivo->path);
			liberar_archivo_abierto(archivo);
			free(paquete->Payload);
			free(paquete);
		}
	}
}

void lockearArchivo(char* path) {
	list_iterate(archivosPorProceso, LAMBDA(void _(PidPath* pp) { if(!strcmp(pp->pathArchivo, path)) sem_wait(&pp->sem);}));
}

void deslockearArchivo(char* path) {
	list_iterate(archivosPorProceso, LAMBDA(void _(PidPath* pp) { if(!strcmp(pp->pathArchivo, path)) sem_post(&pp->sem);}));
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
	} else {
		for(int x = 0; list_size(proceso->segmentos); x++) {
			SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, x);
			if(!strcmp((segmento->idArchivo), path)) {
				posicion = x;
				break;
			}
		}
	}
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
	archivo.segmento = numeroDeSegmento (pid, fid); // Aca iria el segmento donde esta cargado
	archivo.pagina = numeroDePagina(pid, fid); // Aca iria la pagina donde esta cargado
	void *archivo_serializado = DTB_serializar_archivo(&archivo, &desplazamiento_archivo);

	Paquete abrio;
	abrio.Payload = malloc(tam_pid + desplazamiento_archivo);
	memcpy(abrio.Payload, &pid, tam_pid);
	desplazamiento += tam_pid;
	memcpy(abrio.Payload + tam_pid, archivo_serializado, desplazamiento_archivo);
	desplazamiento += desplazamiento_archivo;

	abrio.header = cargar_header(desplazamiento, ABRIR, FM9);
	bool envio = EnviarPaquete(socketFD, &abrio);

	if(envio)
		printf("Paquete a dam archivo abierto enviado %s\n", (char *)abrio.Payload);
	else
		printf("Paquete a dam archivo abierto fallo\n");

	free(archivo_serializado);
	free(abrio.Payload);
}

void consola() {
	char* linea;
	while (true) {
		linea = (char*) readline(">> ");
		if (linea) add_history(linea);
		char** dump = (char**) string_split(linea, " ");
		if (!strcmp(dump[0], "dump")) {
			int idDump = atoi(dump[1]);
			printSEG(idDump);
		}
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
	free(paquete);
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

void liberarMemoriaDesdeHasta(int nroLineaInicio, int nroLineaFin) {
	for(int x = nroLineaInicio; x <= nroLineaFin; x++) {
		strcpy(memoria[x], "NaN");
	}
}

void agregarArchivoYProcesoALista(int pid, char* path) {
	PidPath* pp = malloc(sizeof(PidPath));
	pp->pathArchivo = malloc(strlen(path)+1);
	strcpy(pp->pathArchivo, path);
	pp->idProceso = pid;
	sem_init(&pp->sem, 0, 1);
	list_add(archivosPorProceso, pp);
}

void quitarArchivoYProcesoDeTabla(int pid, char* path) {
	list_remove_by_condition(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid && !strcmp(pp->pathArchivo, path);}));
	//list_remove_and_destroy_by_condition
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

int contarElementosArray(char** array) {
	int contador = 0;
	while (array[contador] != NULL) {
		contador++;
	}
	return contador;
}

int framesSuficientes(int lineasAGuardar) {
	bool disponible(void* frame) {
		return ((Frame*)frame)->disponible == 1;
	}
	int lineasDisponibles = lineasPorFrame * list_count_satisfying(framesMemoria, disponible);
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

////////////////////////////////////////// ↓↓↓  PRINTS ↓↓↓  //////////////////////////////////////////
void printSPA(int pid) {
	ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
	printf("Proceso nro: %d\n", pid);
	if(list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return p->idProceso == pid;}))) {
		for(int x = 0; x < list_size(proceso->segmentos); x++) {
			SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, x);
			printf("Nro. Segmento: %d\nArchivo: %s\nCantidad de lineas: %d\n",
					x, segmento->idArchivo, segmento->cantidadLineas);
			for(int y = 0; y < segmento->cantidadPaginas; y++) {
				Pagina* pagina = list_get(segmento->paginas, y);
				printf("Nro. pagina: %d\nInicio en memoria real: linea %d\nFin en memoria real: linea %d\n",
						y, pagina->inicio, pagina->fin);
			}
		}
	} else printf("No tiene archivos abiertos el proceso con id: %d\n", pid);
}

void printSEG(int pid) {
	ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
	printf("Proceso nro: %d\n", pid);
	printf("Listado de segmentos:\n");
	if(list_find(procesos, LAMBDA(bool _(PidPath* p) {return p->idProceso == pid;}))) {
		for(int x = 0; x < list_size(proceso->segmentos); x++) {
			SegmentoArchivoSEG* segmento = list_get(proceso->segmentos, x);
			printf("Nro. Segmento: %d\nArchivo: %s\nCantidad de lineas: %d\nInicio en memoria real: linea %d\nFin en memoria real: linea %d\n",
					x, segmento->idArchivo, list_size(segmento->lineas), segmento->inicio, segmento->fin);
		}
	} else printf("No tiene archivos abiertos el proceso con id: %d\n", pid);
	imprimirMemoria();
}

void imprimirMemoria(){
	for(int i = 0; i < lineasTotales; i++) {
		printf("posicion de memoria: linea %d, dato: %s\n", i, memoria[i]);
	}
	printf("cantidad lineas libres de memoria: %d\n", cantidadLineasLibresMemoria());
}

void printFrames() {
	for(int x = 0; x < list_size(framesMemoria); x++) {
		Frame* frame = list_get(framesMemoria, x);
		printf("inicio: %d\nfin: %d\ndisponible: %d\n", frame->inicio, frame->fin, frame->disponible);
		for (int y = 0; y < list_size(frame->lineas); y++) {
			printf("elemento: %d con dato: %s\n", y, (char*) list_get(frame->lineas, y));
		}
	}
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
////////////////////////////////////////// ↑↑↑  PRINTS ↑↑↑  //////////////////////////////////////////

void pruebaNuevaPrimitiva(int pid, int pc) {
	char* linea = malloc(MAX_LINEA);
	if(!strcmp(MODO, "SEG")) {
		if(lineaDeUnaPosicionSEG(pid, pc) != NULL) {
			linea = lineaDeUnaPosicionSEG(pid, pc);
			linea = realloc(linea, strlen(linea)+1);
			log_info(logger, "La linea solicitada es: %s\n", linea);
		} else
			log_info(logger, "seg fault", linea);
	} else if(!strcmp(MODO, "TPI")) {
		//
	} else if(!strcmp(MODO, "SPA")) {
		if(lineaDeUnaPosicionSPA(pid, pc) != NULL) {
			linea = lineaDeUnaPosicionSPA(pid, pc);
			linea = realloc(linea, strlen(linea)+1);
			log_info(logger, "La linea solicitada es: %s\n", linea);
		} else
			log_info(logger, "seg fault", linea);
	}
}

int main() {
	crearLogger();
	log_info(logger, "Probando FM9.log");
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	inicializarMemoriaLineas();
	inicializar(MODO);
	pthread_t hiloConsola; //un hilo para la consola
	pthread_create(&hiloConsola, NULL, (void*) consola, NULL);
	ServidorConcurrente(IP_FM9, PUERTO_FM9, FM9, &listaHilos, &end, accion);
	pthread_join(hiloConsola, NULL);
	return EXIT_SUCCESS;
}
