#ifndef DTB_H
#define DTB_H

#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>

//Estructuras
typedef struct DTB{
	u_int32_t gdtPID; // PID = ProcessID
	u_int32_t PC; // PC = Program Counter
	u_int32_t flagInicializacion;
	u_int32_t entrada_salidas;
	t_list *archivosAbiertos;
}__attribute__((packed)) DTB;

typedef struct {
    u_int32_t cantLineas;
    char *path;
}__attribute__((packed)) ArchivoAbierto;

//Funciones

void *DTB_serializar_estaticos(DTB *dtb, int *desplazamiento);

void *DTB_serializar_archivo(ArchivoAbierto *archivo, int *desplazamiento);

void *DTB_serializar_lista(t_list *archivos_abiertos, int *tamanio_total);

void *DTB_serializar(DTB *dtb, int *tamanio_dtb);

void DTB_cargar_estaticos(DTB *dtb, void *data, int *desplazamiento);

ArchivoAbierto *DTB_leer_struct_archivo(void *data, int *desplazamiento);

void DTB_cargar_archivos_abiertos(t_list *archivos_abiertos, void *data, int *desplazamiento);

DTB *DTB_deserializar(void *data);

ArchivoAbierto *_DTB_crear_archivo(int cant_lineas, char *path);
void liberar_archivo_abierto(void *archivo);
void DTB_agregar_archivo(DTB *dtb, int cant_lineas, char *path);
bool find_file(t_list *files, char *path_archivo);
ArchivoAbierto *_DTB_encontrar_archivo(DTB *dtb, char *path_archivo);
void _DTB_remover_archivo(DTB *dtb, char *path);
ArchivoAbierto *DTB_obtener_escriptorio(DTB *dtb);

#endif
