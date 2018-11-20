/*
 * fm9.h
 *
 *  Created on: 8 nov. 2018
 *      Author: utnso
 */

#ifndef FM9_H_
#define FM9_H_

#include "sockets.h"

typedef struct {
	char* idArchivo; //path del archivo
	t_list* lineas; //lista de todas las lineas del archivo en un segmento
	int inicio; //numero posicion de la primer linea en la memoria real
	int desplazamiento; //sizeof del t_list*lineas -1
} SegmentoArchivoSegPur;

typedef struct {
	int disponible; //flag frame disponible, 1=disponible, 0=no disponible
	t_list* lineas; //lista de todas las lineas del archivo en un frame
	int inicio; //nro de linea de inicio
	int fin; //nro de linea de fin (sizeof del t_list*lineas -1)
} Frame;

typedef struct {
	char* idArchivo; //path del archivo
	t_list* paginas; //tabla de paginas cuyo contenido de cada elemento es un nro de frame
	int cantidadPaginas; //cantidad de paginas que representa ese segmento
} SegmentoArchivoSegPag;

typedef struct {
	int idProceso;
	t_list* segmentos; //tabla segmentos/lista de 'SegmentoArchivo' que posee un proceso
} ProcesoArchivo;

//void ejemploCargarArchivoAMemoria();
//void inicializarFramesMemoria();

#endif /* FM9_H_ */