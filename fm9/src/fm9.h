/*
 * fm9.h
 *
 *  Created on: 8 nov. 2018
 *      Author: utnso
 */

#ifndef FM9_H_
#define FM9_H_
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lambda

#include "../../Bibliotecas/sockets.h"

typedef struct {
	char* idArchivo; //path del archivo
	t_list* lineas; //lista de todas las lineas del archivo en un segmento
	int inicio; //posicion de la primer linea en la memoria real
	int fin; //posicion donde se encuentra la ultima linea en la memoria real
} SegmentoArchivoSEG;

typedef struct {
	char* idArchivo; //path del archivo
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
	int idProceso;
	t_list* segmentos; //tabla segmentos/lista de 'SegmentoArchivo' que posee un proceso
} ProcesoArchivo;

typedef struct {
	int idProceso;
	char* pathArchivo;
} PidPath;

void inicializarFramesMemoria();
int cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo);
void cargarArchivoAMemoriaSPA(int pid, char* path, char* archivo);
void printGloriosoSegmentacion(int pid);
void imprimirMemoria();
void printGloriosoSegmentacion(int pid);
void liberarMemoriaDesdeHasta(int nroLineaInicio, int nroLineaFin);
bool archivoAbierto(char* path);
void liberarArchivoSEG(int pid, char* path);
char* lineaDeUnaPosicion(int pid, int pc);

#endif /* FM9_H_ */
