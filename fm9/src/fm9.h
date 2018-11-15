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
	t_list* lineas; //lista de todas las lineas del archivo en un segmento; su list_size es el fin en la ts
	int inicio; //posicion de la memoria real en la que se encuentra la primer linea
} SegmentoArchivo;

typedef struct {
	int idProceso;
	t_list* segmentos; //lista de 'SegmentoArchivo' que posee un proceso
} ProcesoArchivo;

//void ejemploCargarArchivoAMemoria();


#endif /* FM9_H_ */
