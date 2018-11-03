#ifndef DTB_H
#define DTB_H

#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>

//Estructuras
typedef struct DTB{
	u_int32_t gdtPID; // PID = ProcessID
	u_int32_t PC; // PC = Program Counter
	u_int32_t estado;
	u_int32_t flagInicializacion;
	u_int32_t cantidadLineas;
	char *pathEscriptorio;
	t_list *archivosAbiertos;
} DTB;

typedef struct {
    u_int32_t cantLineas;
    char *direccionArchivo;
} ArchivoAbierto;

//Funciones
void *DTB_serializar_archivo(ArchivoAbierto *archivo, int *desplazamiento);

void *DTB_serializar_lista(t_list *archivos_abiertos, int *tamanio_total);

void *DTB_serializar(DTB *dtb, int *tamanio_dtb);

void DTB_cargar_estaticos(DTB *dtb, void *data, int *desplazamiento);

void DTB_cargar_path_escriptorio(DTB *dtb, void *data, int *desplazamiento);

ArchivoAbierto *DTB_leer_struct_archivo(void *data, int *desplazamiento);

void DTB_cargar_archivos_abiertos(t_list *archivos_abiertos, void *data, int *desplazamiento);

DTB *DTB_deserializar(void *data);

#endif