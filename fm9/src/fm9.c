#include "fm9.h"

char *MODO, *IP_FM9;
int PUERTO_FM9, TAMANIO, MAX_LINEA, TAM_PAGINA, socketElDiego, lineasTotales, framesTotales, lineasPorFrame, transfer_size;
char** memoria;

t_list* listaHilos;
t_list* framesMemoria;
t_list* procesos;
t_list* archivosPorProceso; //tabla global de pid-pathArchivo

sem_t escrituraMemoria;
sem_t primitiva;

bool end;

t_log* logger;

////////////////////////////////////////// ↓↓↓  INICIALIZAR ↓↓↓  //////////////////////////////////////////
void crearLogger() {
	logger = log_create("FM9.log", "FM9", true, LOG_LEVEL_INFO);
}

void obtenerValoresArchivoConfiguracion() {
	t_config* arch = config_create("/home/utnso/workspace/tp-2018-2c-Nene-Malloc/fm9/src/FM9.config");

	IP_FM9 = (char*) string_duplicate(config_get_string_value(arch, "IP_FM9"));
	PUERTO_FM9 = config_get_int_value(arch, "PUERTO_FM9");
	MODO = (char*) string_duplicate(config_get_string_value(arch, "MODO"));
	TAMANIO = config_get_int_value(arch, "TAMANIO");
	MAX_LINEA = config_get_int_value(arch, "MAX_LINEA");
	TAM_PAGINA = config_get_int_value(arch, "TAM_PAGINA");

	config_destroy(arch);
}

void inicializarMemoriaLineas() {
	transfer_size = 0;
	sem_init(&escrituraMemoria, 0, 1);
	sem_init(&primitiva, 0, 1);
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
	if(!strcmp(modo, "SEG")) {

	} else if(!strcmp(modo, "TPI")) {

	} else if(!strcmp(modo, "SPA")) {
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

//////////////////////// ↓↓↓ LINEA DE UNA POSICION n ↓↓↓ //////////////////////////////
char* lineaDeUnaPosicionSPA(int pid, int pc) {
	int pcReal = pc-1;
	ProcesoArchivo* unProceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
	SegmentoArchivoSPA* segmento = list_get(unProceso->segmentos, 0);
	if(pcReal < segmento->cantidadLineas) {
		int calculoDePagina = ceil((float) pcReal/(float) lineasPorFrame) - 1; //el -1 es debido a que el index de la primer pagina en la lista es 0
		int calculoDeLinea = pcReal - (lineasPorFrame * (calculoDePagina));
		Pagina* pagina = list_get(segmento->paginas, calculoDePagina);
		printf("lineas por pagina: %d\n", lineasPorFrame);
		printf("nro pagina: %d\n", calculoDePagina);
		printf("inicio pagina: %d\n", pagina->inicio);
		printf("fin pagina: %d\n", pagina->fin);
		for(int x = 0; x < list_size(pagina->lineas); x++) {
			printf("linea: %d contenido: %s\n", x,(char*) list_get(pagina->lineas, x));
		}
		return ((char*) list_get(pagina->lineas, calculoDeLinea));
	} else {
		//segmentation fault
		return NULL;
	}
}

char* lineaDeUnaPosicionSEG(int pid, int pc) {
	int pcReal = pc-1; //porque la linea 1 del archivo esta guardada en la posicion 0
	ProcesoArchivo* unProceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
	SegmentoArchivoSEG* unSegmento = list_get(unProceso->segmentos, 0);
	if(pcReal < unSegmento->fin) {
		return ((char*) list_get(unSegmento->lineas, pcReal));
	} else
		//segmentation fault
		return NULL;
}
//////////////////////// ↑↑↑ LINEA DE UNA POSICION n ↑↑↑ //////////////////////////////

//////////////////////// ↓↓↓ PRIMITIVA ABRIR ↓↓↓ //////////////////////////////////////
void cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo, int socketFD) {
	if(list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return p->idProceso == idProceso && !strcmp(p->pathArchivo, path);}))) {
		printf("ya tiene ese archivo tal proceso\n");
		EnviarDatosTipo(socketFD, FM9, NULL, 0, CARGADO);
	//me fijo si el archivo ya lo posee un proceso y se lo asigno al nuevo. Lo COMPARTEN (VALIDAR EN CLOSE)
	} else if(list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return !strcmp(p->pathArchivo, path);}))) {
		//me traigo el proceso que contiene el archivo pedido
		PidPath* pp = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return !strcmp(p->pathArchivo, path);}));
		ProcesoArchivo* procesoConElArchivo = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* proceso) {return proceso->idProceso == pp->idProceso;}));
		SegmentoArchivoSEG* segmentoConElArchivo = list_find(procesoConElArchivo->segmentos, LAMBDA(bool _(SegmentoArchivoSEG* s) {return !strcmp(s->idArchivo, path);}));
		//si el proceso ya existe se lo agrego, sino creo todas las estructuras
		if(list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == idProceso;}))) {
			ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == idProceso;}));
			SegmentoArchivoSEG* segmentoNuevo = malloc(sizeof(SegmentoArchivoSEG));
			segmentoNuevo->idArchivo = malloc(strlen(path)+1);
			strcpy(segmentoNuevo->idArchivo, path);
			segmentoNuevo->lineas = list_create();
			segmentoNuevo->lineas = list_duplicate(segmentoConElArchivo->lineas); //duda, duplica los punteros?
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
			unSegmentoArchivoSEG->lineas = list_duplicate(segmentoConElArchivo->lineas);
			unSegmentoArchivoSEG->inicio = segmentoConElArchivo->inicio;
			unSegmentoArchivoSEG->fin = segmentoConElArchivo->fin;
			list_add(procesoArchivo->segmentos, unSegmentoArchivoSEG);
			list_add(procesos, procesoArchivo);
		}
		agregarArchivoYProcesoATabla(idProceso, path);
		log_info(logger, "El archivo ya cargado fue agregado a los archivos abiertos del proceso: %d", idProceso);
		EnviarDatosTipo(socketFD, FM9, NULL, 0, CARGADO);
	} else {
		char** arrayLineas = (char**) string_split(archivo, "\n"); //deja un elemento con NULL al final
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
				nuevoSegmento->fin = inicioDeLinea + list_size(lineasArchivo) - 1;
				list_add(unProceso->segmentos, nuevoSegmento);
				for(int x = 0; x < cantidadDeLineas; x++) {
					strcpy(memoria[inicioDeLinea], arrayLineas[x]);
					inicioDeLinea++;
				}
				printf("archivo cargado correctamente\n");
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
				unSegmentoArchivoSEG->fin = list_size(unSegmentoArchivoSEG->lineas) - 1;
				list_add(procesoArchivo->segmentos, unSegmentoArchivoSEG);
				list_add(procesos, procesoArchivo);
				for(int x = 0; x < cantidadDeLineas; x++) {
					strcpy(memoria[inicioDeLinea], arrayLineas[x]);
					inicioDeLinea++;
				}
			}
			log_info(logger, "Archivo cargado con éxito...");
			agregarArchivoYProcesoATabla(idProceso, path);
			free(arrayLineas);
			EnviarDatosTipo(socketFD, FM9, NULL, 0, CARGADO);
		} else {
			printf("No hay espacio suficiente para cargar tal segmento...\n");
			log_info(logger, "No hay espacio suficiente para cargar tal segmento...");
			EnviarDatosTipo(socketFD, FM9, NULL, 0, ESPACIO_INSUFICIENTE_ABRIR);
		}
	}
}

//optimizar repeticion de codigo
void cargarArchivoAMemoriaSPA(int pid, char* path, char* archivo, int socketFD) {
	//ya tiene el archivo el proceso?
	if(list_find(archivosPorProceso, LAMBDA(bool _(PidPath* p) {return p->idProceso == pid && !strcmp(p->pathArchivo, path);}))) {
		printf("ya tiene ese archivo tal proceso\n");
	}
	//el archivo ya existe?
	else if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}))) {
		//obtengo el segmento y sus paginas que contienen al archivo (Se COMPARTE, ver CLOSE)
		PidPath* pidPathConElPath = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* procesoConElPath = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pidPathConElPath->idProceso;}));
		SegmentoArchivoSPA* segmentoConElPath = list_find(procesoConElPath->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
		//el proceso es nuevo o ya existe?
		if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid;}))) {
			//sólo le agrego el archivo
			ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
			SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
			segmento->idArchivo = malloc(strlen(path)+1);
			strcpy(segmento->idArchivo, path);
			segmento->paginas = list_duplicate(segmentoConElPath->paginas); //voy a tener el mismo ptr de los 2 lados? duda
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
			segmento->paginas = list_duplicate(segmentoConElPath->paginas); // duda
			segmento->cantidadPaginas = segmentoConElPath->cantidadPaginas;
			segmento->cantidadLineas = segmentoConElPath->cantidadLineas;
			list_add(procesoNuevo->segmentos, segmento);
			list_add(procesos, procesoNuevo);
		}
		agregarArchivoYProcesoATabla(pid, path);
	} else {
		//el archivo no existe? lo cargo. A su vez el proceso existe?
		char** arrayLineas = (char**) string_split(archivo, "\n"); //deja un elemento con NULL al final
		int cantidadDeLineas = contarElementosArray(arrayLineas);
		if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid;}))) {
			//el proceso existe, cargo el archivo y se lo asigno
			//hay frames libres para cargar el archivo? (espacio en memoria)
			if(framesSuficientes(cantidadDeLineas)) {
				ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
				SegmentoArchivoSPA* segmento = malloc(sizeof(SegmentoArchivoSPA));
				segmento->idArchivo = malloc(strlen(path)+1);
				strcpy(segmento->idArchivo, path);
				segmento->cantidadPaginas = ceil((float) cantidadDeLineas/(float) lineasPorFrame); //si rompe, ver agregar -lm math.h o 'm' a libraries (eclipse)
				int lineas = 0;
				for(int x = 0; x < segmento->cantidadPaginas; x++) {
					Pagina* pagina = malloc(sizeof(Pagina));
					pagina->lineas = list_create();
					Frame* frame = (Frame*) list_get(framesMemoria, primerFrameLibre());
					int inicioFrame = frame->inicio;
					while(lineas < cantidadDeLineas && inicioFrame <= frame->fin) {
						list_add(frame->lineas, arrayLineas[lineas]);
						list_add(pagina->lineas, arrayLineas[lineas]);
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
				list_add(procesos, proceso);
				agregarArchivoYProcesoATabla(pid, path);
			} else {
				printf("No hay frames libres para cargar el archivo indicado\n");
			}
		} else {
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
					pagina->lineas = list_create();
					Frame* frame = (Frame*) list_get(framesMemoria, primerFrameLibre());
					int inicioFrame = frame->inicio;
					while(lineas < cantidadDeLineas && inicioFrame <= frame->fin) {
						list_add(frame->lineas, arrayLineas[lineas]);
						list_add(pagina->lineas, arrayLineas[lineas]);
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
				agregarArchivoYProcesoATabla(pid, path);
			} else {
				printf("No hay frames libres para cargar el archivo indicado\n");
			}
		}
		free(arrayLineas);
	}
}
////////////////////////////// ↑↑↑ PRIMITIVA ABRIR ↑↑↑ ////////////////////////////////////////////////

////////////////////////////// ↓↓↓  PRIMITIVA CLOSE ↓↓↓  //////////////////////////////////////////////
void liberarArchivoSPA(int pid, char* path, int socket) {
	//le pertenece el path al pid?
	if (list_any_satisfy(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path) && pp->idProceso == pid;}))) {
		//hay mas de un proceso con el mismo archivo?
		if (list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}))<2) {
			//lo borro de memoria
			//traigo el segmento y las paginas que ocupa
			PidPath* pidPath = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path) && pp->idProceso == pid;}));
			ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pidPath->idProceso;}));
			SegmentoArchivoSPA* segmento = list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
			//obtengo las paginas que posee el segmento del archivo a liberar
			for(int x = 0; x < segmento->cantidadPaginas; x++) {
				Pagina* pagina = list_get(segmento->paginas, x);
				//busco el frame en memoria principal por linea de inicio
				Frame* frame = list_find(framesMemoria, LAMBDA(bool _(Frame* f) {return f->inicio == pagina->inicio;}));
				frame->disponible = 1;
				for(int y = frame->inicio; y <= frame->fin; y++) {
					strcpy(memoria[y], "NaN");
				}
			}
			list_remove_by_condition(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid && !strcmp(pp->pathArchivo, path);}));
			//Se destruye/libera: segmento que contiene el path, la lista de paginas que tenia dentro, y la lista de lineas que habia dentro de cada pagina
			list_remove_and_destroy_by_condition(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}), LAMBDA(void _(SegmentoArchivoSPA* seg) {free(seg->idArchivo); list_destroy_and_destroy_elements(seg->paginas, LAMBDA(void _(Pagina* p) {list_destroy_and_destroy_elements(p->lineas, LAMBDA(void _(char* linea) {free(linea);})); free(p);}));}));
			printf("Archivo liberado correctamente\n");
		} else {
		printf("el archivo no se encuentra abierto por tal pid\n");
		}
	}
}

void liberarArchivoSEG(int pid, char* path, int socket) {
	void borrarSegmento(SegmentoArchivoSEG* seg) {
		//si tengo mas de 1 proceso con un archivo, sólo se lo quito al pedido, no lo limpio de memoria
		if (list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path) && pp->idProceso == pid;}))<2)
			liberarMemoriaDesdeHasta(seg->inicio, seg->fin);
		list_destroy_and_destroy_elements(seg->lineas, LAMBDA(void _(char* linea) {free(linea);}));
		free(seg->idArchivo);
	}
	if (list_any_satisfy(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}))) {
		ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
		list_remove_and_destroy_by_condition(proceso->segmentos,
				LAMBDA(bool _(SegmentoArchivoSEG* s) {return !strcmp(s->idArchivo, path);}),
				(void*) borrarSegmento);
		printf("Archivo %s liberado\n", path);
		list_remove_by_condition(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid && !strcmp(pp->pathArchivo, path);}));
	} else {
		printf("El archivo no se encuentra abierto\n");
	}
}
//////////////////////////////// ↑↑↑  PRIMITIVA CLOSE ↑↑↑  //////////////////////////////////////////

////////////////////////////// ↓↓↓ PRIMITIVA ASIGNAR ↓↓↓ ////////////////////////////////////////////
void asignarSPA(int pid, char* path, int pos, char* dato, int socket) {
	int posReal = pos - 1; //dado que la linea 1 de un archivo, se persiste la posicion 0
	//posee el archivo?
	if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid && !strcmp(pp->pathArchivo, path);}))) {
		ProcesoArchivo* proceso = (ProcesoArchivo*) list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
		SegmentoArchivoSPA* segmento = (SegmentoArchivoSPA*) list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
		//esta dentro de su rango de memoria?
		int calculoDePagina = ceil((float) posReal/(float) lineasPorFrame) -1;
		if(calculoDePagina < segmento->cantidadPaginas && pos <= segmento->cantidadLineas) {
			//tengo el numero de pagina y la linea donde se ubica la posicion recibida
			Pagina* pag = list_get(segmento->paginas, calculoDePagina);
			int calculoDeLinea = posReal - (calculoDePagina * lineasPorFrame);
			//busco el frame en memoria principal por linea de inicio
			Frame* frame = list_find(framesMemoria, LAMBDA(bool _(Frame* f) {return f->inicio == pag->inicio;}));
			list_replace(pag->lineas, calculoDeLinea, dato);
			list_replace(frame->lineas, calculoDeLinea, dato);
			strcpy(memoria[frame->inicio + calculoDeLinea], dato);
		} else {
			printf("no pertenece a su rango de memoria\n");
		}
	} else {
		printf("el pid no posee tal archivo\n");
	}
}

void asignarSEG(int pid, char* path, int pos, char* dato, int socketFD) {
	int posReal = pos - 1; //dado que la linea 1 de un archivo, se persiste la posicion 0
	//posee el archivo?
	if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return pp->idProceso == pid && !strcmp(pp->pathArchivo, path);}))) {
		//la posicion solicitada le pertenece?
		ProcesoArchivo* proceso = (ProcesoArchivo*) list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
		SegmentoArchivoSEG* segmento = (SegmentoArchivoSEG*) list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSEG* s) {return !strcmp(s->idArchivo, path);}));
		if((segmento->inicio + posReal) < segmento->fin) {
			list_replace(segmento->lineas, posReal, dato);
			strcpy(memoria[segmento->inicio + posReal], dato);
		} else {
			printf("Segmentation fault (cored dumped)\n");
			EnviarDatosTipo(socketFD, FM9, NULL, 0, ABORTAR);
		}
	} else {
		printf("el proceso no posee tal archivo\n");
		EnviarDatosTipo(socketFD, FM9, NULL, 0, SUCCESS);
	}
}
//////////////////////// ↑↑↑ PRIMITIVA ASIGNAR ↑↑↑ //////////////////////////////////////////

//////////////////////// ↓↓↓ PRIMITIVA FLUSH ↓↓↓ ////////////////////////////////////////////
void flushSEG(char* path, int socketFD) {
	//archivo abierto?
	if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}))) {
		PidPath* pp = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pp->idProceso;}));
		SegmentoArchivoSEG* segmento = list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSEG* s) {return !strcmp(s->idArchivo, path);}));
		char* texto = malloc(MAX_LINEA * list_size(segmento->lineas));
		int posicionPtr = 0;
		for(int x = segmento->inicio; x < segmento->inicio + list_size(segmento->lineas); x++) {
			//transformo la lista de lineas en un char* concatenado por \n
			strcpy(texto, list_get(segmento->lineas, x));
			size_t longitud = strlen(texto);
			*(texto + longitud) = '\n';
			*(texto + longitud + 1) = '\0';
			texto += longitud + 1;
			posicionPtr += longitud + 1;
		}
		size_t longitud = strlen(texto);
		*(texto + longitud) = '\n';
		*(texto + longitud + 1) = '\n';
		*(texto + longitud + 2) = '\0';
		texto = texto-posicionPtr;
		texto = realloc(texto, strlen(texto)+1);
		printf("%s", texto);
	} else {
		printf("el archivo no se encuentra abierto\n");
	}
}

void flushSPA(char* path, int socketFD) {
	//archivo abierto?
	if(list_count_satisfying(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}))) {
		PidPath* pp = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pp->idProceso;}));
		SegmentoArchivoSPA* segmento = list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(s->idArchivo, path);}));
		char* texto = malloc(MAX_LINEA * (segmento->cantidadPaginas * lineasPorFrame));
		int lineas = 0;
		int posicionPtr = 0;
		for(int x = 0; x < segmento->cantidadPaginas; x++) {
			Pagina* pagina = list_get(segmento->paginas, x);
			int inicioPagina = pagina->inicio;
			int contador = 0;
			while(lineas < segmento->cantidadLineas && inicioPagina <= pagina->fin) {
				strcpy(texto, (char*) list_get(pagina->lineas, contador));
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
		switch (paquete.header.emisor) {
		case ELDIEGO: {
			if (paquete.header.tipoMensaje != ESHANDSHAKE && paquete.header.tamPayload > 0) {
				paquete.Payload = malloc(paquete.header.tamPayload);
				recibir_partes(socketFD, paquete.Payload, paquete.header.tamPayload);
			}
			manejar_paquetes_diego(&paquete, socketFD);
		} break;

		case CPU: {
			if (paquete.header.tamPayload > 0) {
				paquete.Payload = malloc(paquete.header.tamPayload);
				RecibirDatos(paquete.Payload, socketFD, paquete.header.tamPayload);
			}
			manejar_paquetes_CPU(&paquete, socketFD);
		} break;
		default:
			log_warning(logger, "No se reconoce el emisor %d", paquete.header.emisor);
		} break;
	}
	// Si sale del while hubo error o desconexion
	if (paquete.Payload != NULL) free(paquete.Payload);
	close(socketFD);
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
		} break;

		case ABRIR: {
			sem_wait(&primitiva);
			u_int32_t pid;
			int tam_pid = sizeof(u_int32_t);
			memcpy(&pid, paquete->Payload, tam_pid);
			int tam_desplazado = tam_pid;
			char *path = string_deserializar(paquete->Payload, &tam_desplazado);
			char *archivo = string_deserializar(paquete->Payload, &tam_desplazado);
			free(paquete->Payload);

			if(!strcmp(MODO, "SEG")) {
				cargarArchivoAMemoriaSEG(pid, path, archivo, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				cargarArchivoAMemoriaSPA(pid, path, archivo, socketFD);
			}
		}
		sem_wait(&primitiva);
		break;

		case FLUSH: {
			//el payload viene con el path
			sem_wait(&primitiva);
			char* path = malloc(strlen(paquete->Payload)+ 1);
			strcpy(path, paquete->Payload);
			free(paquete);
			if(!strcmp(MODO, "SEG")) {
				flushSEG(path, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				flushSPA(path, socketFD);
			}
		}
		sem_wait(&primitiva);
		break;
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
		} break;

		case NUEVA_PRIMITIVA: {
			//el payload debe ser: int(pid)+int(pc)
			int pid, pc;
			memcpy(&pid, paquete->Payload, sizeof(pid));
			paquete->Payload += sizeof(pid);
			memcpy(&pc, paquete->Payload, sizeof(pc));
			char* linea = malloc(MAX_LINEA);
			if(!strcmp(MODO, "SEG")) {
				if(lineaDeUnaPosicionSEG(pid, pc) != NULL) {
					linea = lineaDeUnaPosicionSEG(pid, pc);
					linea = realloc(linea, strlen(linea)+1);
					EnviarDatosTipo(socketFD, CPU, linea, strlen(linea)+1, LINEA_PEDIDA);
				} else
					//segmentation fault
					EnviarDatosTipo(socketFD, CPU, NULL, 0, ERROR);
			} else if(!strcmp(MODO, "TPI")) {
				//
			} else if(!strcmp(MODO, "SPA")) {
				if(lineaDeUnaPosicionSEG(pid, pc) != NULL) {
					linea = lineaDeUnaPosicionSPA(pid, pc);
					linea = realloc(linea, strlen(linea)+1);
					EnviarDatosTipo(socketFD, CPU, linea, strlen(linea)+1, LINEA_PEDIDA);
				} else
					//segmentation fault
					EnviarDatosTipo(socketFD, CPU, NULL, 0, ERROR);
			}
			free(linea);
		} break;

		case ASIGNAR: {
			//el payload debe ser int+char\0+int+char\0
			sem_wait(&primitiva);
			int pid, pos;
			memcpy(&pid, paquete->Payload, sizeof(pid));
			paquete->Payload += sizeof(pid);
			char* path = malloc(strlen(paquete->Payload)+1);
			strcpy(path, paquete->Payload);
			paquete->Payload += strlen(path)+1;
			memcpy(&pos, paquete->Payload, sizeof(pos));
			paquete->Payload += sizeof(pos);
			char* dato = malloc(strlen(paquete->Payload)+1);
			strcpy(dato, paquete->Payload);
			if(!strcmp(MODO, "SEG")) {
				asignarSEG(pid, path, pos, dato, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				asignarSEG(pid, path, pos, dato, socketFD);
			}
			free(path);
			free(dato);
			sem_post(&primitiva);
		} break;

		case CLOSE: {
			//el payload debe ser int+char\0
			int pid;
			memcpy(&pid, paquete->Payload, sizeof(pid));
			paquete->Payload += sizeof(pid);
			char* path = malloc(paquete->header.tamPayload-sizeof(pid));
			strcpy(path, paquete->Payload);
			if(!strcmp(MODO, "SEG")) {
				liberarArchivoSEG(pid, path, socketFD);
			} else if(!strcmp(MODO, "TPI")) {

			} else if(!strcmp(MODO, "SPA")) {
				liberarArchivoSPA(pid, path, socketFD);
			}
			free(path);
		} break;
	}
}

int contar_lineas (char *file) {
	int i, lineas = 0;
	for (i = 0; i < strlen(file); i++) 	{
		if(file[i] == '\n') lineas++;
	}
	return lineas-1;
}

//implementado solo para SEG y SPA por ahora
int contarLineas(char* path) {
	if(!strcmp(MODO, "SEG")) {
		PidPath* pidPath = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pidPath->idProceso;}));
		SegmentoArchivoSEG* segmento = list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(segmento->idArchivo, path);}));
		return list_size(segmento->lineas);
	} else {
		PidPath* pidPath = list_find(archivosPorProceso, LAMBDA(bool _(PidPath* pp) {return !strcmp(pp->pathArchivo, path);}));
		ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pidPath->idProceso;}));
		SegmentoArchivoSPA* segmento = list_find(proceso->segmentos, LAMBDA(bool _(SegmentoArchivoSPA* s) {return !strcmp(segmento->idArchivo, path);}));
		Pagina* pag = list_get(segmento->paginas, segmento->cantidadPaginas - 1);
		int cantLineas = lineasPorFrame * (segmento->cantidadPaginas - 1);
		for(int x = 0; x < lineasPorFrame; x++) {
			if(!strcmp(list_get(pag->lineas, x), "NaN")) cantLineas++;
		}
		return cantLineas;
	}
}

int numeroDeSegmento(int pid, char* path) {
	int posicion = -1;
	ProcesoArchivo* proceso = list_find(procesos, LAMBDA(bool _(ProcesoArchivo* p) {return p->idProceso == pid;}));
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
	return 1;
}

//void enviar_abrio_a_dam(int socketFD, u_int32_t pid, char *fid, char *file) {
//	int desplazamiento = 0;
//	int desplazamiento_archivo = 0;
//	int tam_pid = sizeof(u_int32_t);
//
//	ArchivoAbierto archivo;
//	archivo.path = string_duplicate(fid);
//	archivo.cantLineas = contar_lineas(fid);
//	archivo.segmento = numeroDeSegmento(int pid, char* path); // Aca iria el segmento donde esta cargado
//	archivo.pagina = numeroDePagina(int pid, char* path); // Aca iria la pagina donde esta cargado
//	void *archivo_serializado = DTB_serializar_archivo(&archivo, &desplazamiento_archivo);
//
//	Paquete abrio;
//	abrio.Payload = malloc(tam_pid + desplazamiento_archivo);
//	memcpy(abrio.Payload, &pid, tam_pid);
//	desplazamiento += tam_pid;
//	memcpy(abrio.Payload + tam_pid, archivo_serializado, desplazamiento_archivo);
//	desplazamiento += desplazamiento_archivo;
//
//	abrio.header = cargar_header(desplazamiento, ABRIR, FM9);
//	bool envio = EnviarPaquete(socketFD, &abrio);
//
//	if(envio)
//		printf("Paquete a dam archivo abierto enviado %s\n", (char *)abrio.Payload);
//	else
//		printf("Paquete a dam archivo abierto fallo\n");
//
//	free(archivo_serializado);
//	free(abrio.Payload);
//}

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
	bool valor_retorno=EnviarPaquete(socketFD, paquete);
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

void agregarArchivoYProcesoATabla(int pid, char* path) {
	PidPath* pp = malloc(sizeof(PidPath));
	pp->pathArchivo = malloc(strlen(path)+1);
	strcpy(pp->pathArchivo, path);
	pp->idProceso = pid;
	list_add(archivosPorProceso, pp);
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
	if(list_find(procesos, LAMBDA(bool _(PidPath* p) {return p->idProceso == pid;}))) {
		for(int x = 0; x < list_size(proceso->segmentos); x++) {
			SegmentoArchivoSPA* segmento = list_get(proceso->segmentos, x);
			printf("Nro. Segmento: %d\nArchivo: %s\nCantidad de lineas: %d\n",
					x, segmento->idArchivo, segmento->cantidadLineas);
			for(int y = 0; y < segmento->cantidadPaginas; y++) {
				Pagina* pagina = list_get(segmento->paginas, y);
				printf("Nro. pagina: %d\nCantidad lineas: %d\nInicio en memoria real: linea %d\nFin en memoria real: linea %d\n",
						y, list_size(pagina->lineas), pagina->inicio, pagina->fin);
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
