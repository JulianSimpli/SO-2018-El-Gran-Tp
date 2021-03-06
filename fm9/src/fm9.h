/*
 * fm9.h
 *
 *  Created on: 8 nov. 2018
 *      Author: utnso
 */

#ifndef FM9_H_
#define FM9_H_

//#include "sockets.h"
//#include "dtb.h"
//#include "loggers.h"
#include <commons/collections/list.h>
#include "../../Bibliotecas/sockets.h"
#include "../../Bibliotecas/helper.h"
#include "../../Bibliotecas/dtb.h"
#include "../../Bibliotecas/loggers.h"

typedef struct {
	char* idArchivo; //path del archivo
	t_list* lineas; //lista de todas las lineas del archivo en un segmento
	int inicio; //posicion de la primer linea en la memoria real
	int fin; //posicion donde se encuentra la ultima linea en la memoria real
} SegmentoArchivoSEG;

typedef struct {
	char* idArchivo; //path del archivo
	int cantidadLineas;
	t_list* paginas; //tabla de paginas cuyo contenido de cada elemento es un frame
	int cantidadPaginas; //cantidad de paginas que representa ese segmento
} SegmentoArchivoSPA;

typedef struct {
	int disponible; //flag frame disponible, 1=disponible, 0=no disponible
	t_list* lineas; //lista de todas las lineas del archivo en un frame
	int inicio; //nro de linea de inicio
	int fin; //nro de linea de fin (sizeof del t_list*lineas -1)
} Frame;

typedef struct {
	int numeroFrame;
} Pagina;

typedef struct {
	int idProceso;
	t_list* segmentos; //tabla/lista de 'SegmentoArchivo' que posee un proceso
} ProcesoArchivo;

//funciones para SEG
void cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo, int socketFD);
int liberarArchivoSEG(int pid, int seg, int socketFD);
char* lineaDeUnaPosicionSEG(int pid, int pc);
void asignarSEG(int pid, int seg, int pos, char* dato, int socketFD);
void flushSEG(int seg, int pid, int socketFD);
int numeroDeSegmento(int pid, char* path);

//funciones para SPA
void cargarArchivoAMemoriaSPA(int pid, char* path, char* archivo, int socketFD);
int liberarArchivoSPA(int pid, int seg, int socketFD);
char* lineaDeUnaPosicionSPA(int pid, int pc);
void asignarSPA(int pid, int seg, int pos, char* dato, int socketFD);
void flushSPA(int seg, int pid, int socketFD);
int numeroDePagina(int pid, char* path);

//funciones de conexiones
void enviar_flush_a_dam(int socketFD, char *path, char *archivo);
void enviar_abrio_a_dam(int socketFD, u_int32_t pid, char* fid, char* file);
void manejar_paquetes_CPU(Paquete* paquete, int socketFD);
void manejar_paquetes_diego(Paquete* paquete, int socketFD);
void EnviarHandshakeELDIEGO(int socketFD);

//funciones de memoria
void liberarMemoriaDesdeHasta(int nroLineaInicio, int nroLineaFin);
int lineasLibresConsecutivasMemoria(int lineasAGuardar);
int primerFrameLibre();
int framesSuficientes(int lineasAGuardar);

//funciones de logs
void logProcesoSEG(int pid);
void logProcesoSPA(int pid);
void logEstadoFrames();
void logMemoria();

//otros
void consola();
char** cargarArray(char* archivo, int* cantidadLineas);
int contar_lineas (char *file);

#endif /* FM9_H_ */
