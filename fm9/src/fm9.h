/*
 * fm9.h
 *
 *  Created on: 8 nov. 2018
 *      Author: utnso
 */

#ifndef FM9_H_
#define FM9_H_

#include "sockets.h"
#include "dtb.h"
//#include <commons/collections/list.h>
//#include "../../Bibliotecas/sockets.h"
//#include "../../Bibliotecas/helper.h"
//#include "../../Bibliotecas/dtb.h"

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
	t_list* lineas;
	int inicio;
	int fin;
} Pagina;

typedef struct {
	int idProceso;
	t_list* segmentos; //tabla segmentos/lista de 'SegmentoArchivo' que posee un proceso
} ProcesoArchivo;

typedef struct {
	int idProceso;
	char* pathArchivo;
} PidPath;

void consola();
void inicializarFramesMemoria();
void cargarArchivoAMemoriaSEG(int idProceso, char* path, char* archivo, int socketFD);
void cargarArchivoAMemoriaSPA(int pid, char* path, char* archivo, int socketFD);
void printSEG(int pid);
void printSPA(int pid);
void imprimirMemoria();
void liberarMemoriaDesdeHasta(int nroLineaInicio, int nroLineaFin);
bool archivoAbierto(char* path);
void liberarArchivoSEG(int pid, char* path, int socketFD);
void liberarArchivoSPA(int pid, char* path, int socketFD);
char* lineaDeUnaPosicionSEG(int pid, int pc);
char* lineaDeUnaPosicionSPA(int pid, int pc);
void enviar_abrio_a_dam(int socketFD, u_int32_t pid, char* fid, char* file);
void manejar_paquetes_diego(Paquete* paquete, int socketFD);
void manejar_paquetes_CPU(Paquete* paquete, int socketFD);
void asignarSEG(int pid, char* path, int pos, char* dato, int socketFD);
void asignarSPA(int pid, char* path, int pos, char* dato, int socketFD);
void flushSEG(char* path, int socketFD);
void flushSPA(char* path, int socketFD);

#endif /* FM9_H_ */
