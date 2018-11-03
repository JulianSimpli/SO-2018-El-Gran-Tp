
#include "dtb.h"

void *DTB_serializar_archivo(ArchivoAbierto *archivo, int *desplazamiento)
{
    int tamanio = sizeof(u_int32_t);
    u_int32_t tamanio_direccion = strlen(archivo->direccionArchivo);

    void *serializado = malloc(sizeof(ArchivoAbierto) + tamanio + tamanio_direccion);

	memcpy(serializado + *desplazamiento, &archivo->cantLineas, tamanio);
	*desplazamiento += tamanio;

    memcpy(serializado + *desplazamiento, &tamanio_direccion, tamanio);
    *desplazamiento += tamanio;

    memcpy(serializado + *desplazamiento, archivo->direccionArchivo, tamanio_direccion);
    *desplazamiento += tamanio_direccion;

    return serializado;
}

void *DTB_serializar_lista(t_list *archivos_abiertos, int *tamanio_total)
{
    u_int32_t cant_elementos = list_size(archivos_abiertos);
    int i;
    void *serializado = malloc(0);

    for(i = 0; i < cant_elementos; i++) {
        ArchivoAbierto *archivo = list_get(archivos_abiertos, i);
        int tamanio_serializado = 0; 
        void *archivo_serializado = DTB_serializar_archivo(archivo, &tamanio_serializado);

        serializado = realloc(serializado, *tamanio_total + tamanio_serializado);
        memcpy(serializado + *tamanio_total, archivo_serializado, tamanio_serializado);


        *tamanio_total += tamanio_serializado;
    }

    return serializado;
}


void *DTB_serializar(DTB *dtb, int *tamanio_dtb)
{
	void *payload = malloc(sizeof(DTB));
	int desplazamiento = 0;
	int tamanio = sizeof(u_int32_t);
	memcpy(payload, &dtb->gdtPID, tamanio);
	desplazamiento += tamanio;			
	memcpy(payload + desplazamiento, &dtb->PC, tamanio);
	desplazamiento += tamanio;			
	memcpy(payload + desplazamiento, &dtb->estado, tamanio);
	desplazamiento += tamanio;			
	memcpy(payload + desplazamiento, &dtb->flagInicializacion, tamanio);
	desplazamiento += tamanio;			
	memcpy(payload + desplazamiento, &dtb->cantidadLineas, tamanio);
	desplazamiento += tamanio;			

	u_int32_t len = strlen(dtb->pathEscriptorio);
    payload = realloc(payload, desplazamiento + tamanio);
	memcpy(payload + desplazamiento, &len, tamanio);
	desplazamiento += tamanio;			

    payload = realloc(payload, desplazamiento + len);
	memcpy(payload + desplazamiento, dtb->pathEscriptorio, len);
	desplazamiento += len;			

    if(dtb->archivosAbiertos == NULL) {
        u_int32_t vacia = 0;
    	memcpy(payload + desplazamiento, &vacia, tamanio);
	    desplazamiento += tamanio;			
        *tamanio_dtb = desplazamiento;
        return payload;
    }

    u_int32_t cant_archivos = list_size(dtb->archivosAbiertos);
    payload = realloc(payload, desplazamiento + tamanio);
    memcpy(payload + desplazamiento, &cant_archivos, tamanio);
	desplazamiento += tamanio;			

    int tamanio_lista_serializada = 0;
    void *lista_serializada = DTB_serializar_lista(dtb->archivosAbiertos, &tamanio_lista_serializada);
    payload = realloc(payload, desplazamiento + tamanio_lista_serializada);
    memcpy(payload + desplazamiento, lista_serializada, tamanio_lista_serializada);
    desplazamiento += tamanio_lista_serializada;

    *tamanio_dtb = desplazamiento;
    return payload;
}

void DTB_cargar_estaticos(DTB *dtb, void *data, int *desplazamiento)
{
    size_t tamanio = sizeof(u_int32_t) * 5;
	memcpy(dtb, data, tamanio);
    *desplazamiento += tamanio; 
}

void DTB_cargar_path_escriptorio(DTB *dtb, void *data, int *desplazamiento)
{
    u_int32_t len;
	memcpy(&len, data + *desplazamiento, sizeof(u_int32_t));
    *desplazamiento += sizeof(u_int32_t);
    dtb->pathEscriptorio = malloc(len + 1);
	memcpy(dtb->pathEscriptorio, data + *desplazamiento, len);
    dtb->pathEscriptorio[len] = '\0';
    *desplazamiento += len; 
}

ArchivoAbierto *DTB_leer_struct_archivo(void *data, int *desplazamiento)
{
    ArchivoAbierto *archivo = malloc(sizeof(ArchivoAbierto));

    size_t tamanio = sizeof(u_int32_t);

    memcpy(&archivo->cantLineas, data + *desplazamiento, tamanio);    
    *desplazamiento += tamanio;

    u_int32_t tamanio_direccion;
    memcpy(&tamanio_direccion, data + *desplazamiento, tamanio);    
    *desplazamiento += tamanio;

    archivo->direccionArchivo = malloc(tamanio_direccion + 1);
    memcpy(archivo->direccionArchivo, data + *desplazamiento, tamanio_direccion);    
    archivo->direccionArchivo[tamanio_direccion] = '\0';
    *desplazamiento += tamanio_direccion;

    return archivo;
}

void DTB_cargar_archivos_abiertos(t_list *archivos_abiertos, void *data, int *desplazamiento)
{
    u_int32_t cant_elementos;
    memcpy(&cant_elementos, data + *desplazamiento, sizeof(u_int32_t));
    *desplazamiento += sizeof(u_int32_t);

    while(cant_elementos) {
        ArchivoAbierto *archivo = DTB_leer_struct_archivo(data, desplazamiento);
	    list_add(archivos_abiertos, archivo);
        cant_elementos--;
    }
}

DTB *DTB_deserializar(void *data)
{
    DTB *dtb = malloc(sizeof(DTB));
    int desplazamiento = 0;
    DTB_cargar_estaticos(dtb, data, &desplazamiento);
    DTB_cargar_path_escriptorio(dtb, data, &desplazamiento);
    dtb->archivosAbiertos = list_create();
    DTB_cargar_archivos_abiertos(dtb->archivosAbiertos, data, &desplazamiento);
    return dtb;
}
