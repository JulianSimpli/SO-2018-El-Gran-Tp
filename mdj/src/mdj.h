#ifndef CLIENTE_H_
#define CLIENTE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/error.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <dirent.h>
#include "../../Bibliotecas/sockets.h"

//Logger y config globales
t_log *logger;
t_config *config;
t_bitarray *bitarray;

int transfer_size;

typedef struct archivo_metadata
{
    char *nombre;
    int tamanio;
    t_list *bloques;
} Archivo_metadata;

void leer_config(char *path);
void inicializar_log(char *program);
void inicializar(char **argv);
int interpretar(char *linea);
int escuchar_conexiones();
Mensaje *recibir_mensaje(int conexion);
void *atender_dam();
void *atender_peticion(void *args);
void interpretar_paquete(Paquete *paquete);
void handshake(int, int);
void enviar_mensaje(Mensaje);
void marcarBitarray(t_config *metadata);
int create_block(int index, int cantidad_bytes);
int destroy_block(int index);
t_list *get_space(int size);
char *convertir_bloques_a_string(Archivo_metadata *fm);
void cargar_bitarray();
void imprimir_directorios(char *);
void enviar_error(Tipo tipo);
void cargar_metadata();
char *leer_bitmap();
char *mnt_path;
char *file_path;
char *blocks_path;
char *metadata_path;
int cantidad_bloques;
int tamanio_bloques;
int socket_dam;
char *current_path;
int retardo;
int transfer_size;
sem_t sem_bitarray;

int validar_archivo(Paquete *paquete, char *);
void crear_archivo(Paquete *paquete);
void obtener_datos(Paquete *paquete);
void guardar_datos(Paquete *paquete);
void borrar_archivo(Paquete *paquete);

//Estas son las funciones que ejecuta cada comando respectivamente
//Todas tienen que tener la misma firma o se podria ampliar recibiendo void*
void com_help(char *linea);
void com_exit(char *linea);

void enviar_mensaje(Mensaje mensaje)
{
    void *buffer = malloc(sizeof(Paquete));

    int mensaje_enviado = send(mensaje.socket, buffer, sizeof(Paquete), 0);

    if (mensaje_enviado < 0)
        log_error(logger, "No pudo enviar el mensaje");
}

/**
 * Se encarga de crear el socket y mandar el primer mensaje
 * Devuelve el socket 
 */
void handshake_dam()
{
    socket_dam = escuchar_conexiones();
    Paquete paquete;
    recibir_paquete(socket_dam, &paquete);
    if (paquete.header.tipoMensaje != ESHANDSHAKE)
        _exit_with_error(socket_dam, "No se logro el handshake", NULL);
    memcpy(&transfer_size, paquete.Payload, INTSIZE);
    EnviarHandshake(socket_dam, MDJ);
}

void imprimir_directorios(char *path_a_imprimir)
{
    DIR *dir;
    struct dirent *ent;

    dir = opendir(path_a_imprimir);
    while ((ent = readdir(dir)) != NULL)
    {
        printf("%s\n", ent->d_name);
    }
}

void com_list(char **parametros)
{
    printf("The folder has this currents files: \n");

    char *punto_montaje;
    strcpy(punto_montaje, mnt_path);
    char *path = parametros[1];
    if (path == NULL)
    {
        imprimir_directorios(punto_montaje);
    }
    else
    {
        if (path[0] == '/')
        {
            imprimir_directorios(path);
        }
        else
        {
            strcat(punto_montaje, path);
            imprimir_directorios(punto_montaje);
        }
    }
    //fijarse en que path está y mostrar que es directorio y que es archivo
}

void com_cat(char **parametros)
{
    char *directorio_actual;
    //cd tiene que actualizar el directorio actual
    strcpy(directorio_actual, current_path);
    char *nuevo_path = parametros[1];

    if (nuevo_path[0] == '/')
    {
        directorio_actual = nuevo_path;
    }
    else
    {
        strcat(directorio_actual, nuevo_path);
    }

    FILE *f = fopen(directorio_actual, "rb");

    if (f == NULL)
        perror("Fallo al abrir el archivo: ");

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *cadena_a_devolver = malloc(size + 1);
    fread(cadena_a_devolver, size, 1, f);
    fclose(f);
    cadena_a_devolver[size] = '\0';
    printf("%s\n", cadena_a_devolver);
    printf("CAT\n");
}

void com_cd(char *linea)
{
    printf("CD\n");
    system("cd /home/daniel");
}

void com_md5(char *linea)
{
    printf("MD5\n");
}

void com_exit(char *linea)
{
    printf("BYE\n");
    free(linea);
    exit_gracefully(0);
}

typedef void (*DoRunTimeChecks)();

typedef struct
{
    char *name;           /* User printable name of the function. */
    DoRunTimeChecks func; /* Function to call to do the job. */
    char *doc;            /* Documentation for this function.  */
} COMMAND;

COMMAND commands[] = {
    {"cd", com_cd, "Change to directory DIR"},
    {"cat", com_cat, "Concatenate the content of FILE to stdout"},
    {"help", com_help, "Display this text"},
    {"?", com_help, "Synonym for `help'"},
    {"list", com_list, "List files in DIR"},
    {"ls", com_list, "Synonym for `list'"},
    {"md5", com_md5, "Returns md5 representation of FILE"},
    {"exit", com_exit, "Exit the program"},
    {(char *)NULL, (DoRunTimeChecks)NULL, (char *)NULL}};

void com_help(char *linea)
{
    int i = 0;
    size_t command_size = sizeof(commands) / sizeof(commands[0]) - 1;
    printf("MDJ reponde a los siguientes %d comandos:\n", (int)command_size);
    for (i = 0; i < command_size; i++)
    {
        printf("%s: %s\n", commands[i].name, commands[i].doc);
    }
}

Mensaje *recibir_mensaje(int conexion)
{
    Mensaje *buffer = (Mensaje *)malloc(sizeof(Mensaje));
    read(conexion, buffer, sizeof(Mensaje));

    if (buffer == NULL)
    {
        log_error(logger, "Error en la lectura del Mensaje");
        exit_gracefully(2);
    }

    return buffer;
}

char *get_block_full_path(int index)
{
    char *block_path = malloc(strlen(blocks_path) + 1);
    strcpy(block_path, blocks_path);
    sprintf(block_path, "%s/%d.bin", block_path, index);
    return block_path;
}

int crear_bloques(t_list *lista_bloques, int *cantidad_bytes)
{
    for (int i = 0; i < list_size(lista_bloques); i++)
    {
        create_block((__intptr_t)list_get(lista_bloques, i), *cantidad_bytes);
        *cantidad_bytes -= tamanio_bloques;
    }
}

int create_block_file(char *path, int cantidad_bytes)
{
    FILE *f = fopen(path, "w+");

    if (f == NULL)
        return 1;

    while (cantidad_bytes)
    {
        fputs("\n\0", f);
        cantidad_bytes--;
    }
    fclose(f);
}

int create_block(int index, int cantidad_bytes)
{
    char *block_path = get_block_full_path(index);

	sem_wait(&sem_bitarray);	
    bitarray_set_bit(bitarray, index);
	sem_post(&sem_bitarray);	

    return create_block_file(block_path, cantidad_bytes) != 0;
}

int destroy_block(int index)
{
    char *block_path = get_block_full_path(index);

	sem_wait(&sem_bitarray);	
    bitarray_clean_bit(bitarray, index);
	sem_post(&sem_bitarray);	

    return 0;
}

int guardar_metadata(Archivo_metadata *fm)
{
    FILE *f = fopen(fm->nombre, "w+");

    if (f == NULL)
    {
        log_error(logger, "%s", strerror(errno));
        return 1;
    }

    fprintf(f, "%s%d\n", "TAMANIO=", fm->tamanio);
    fprintf(f, "%s%s\n", "BLOQUES=", convertir_bloques_a_string(fm));

    fclose(f);
    return 0;
}

char *convertir_bloques_a_string(Archivo_metadata *fm)
{
    char *bloques = malloc(256);
    int i;
    sprintf(bloques, "[%d", (int)(intptr_t)list_get(fm->bloques, 0));

    for (i = 1; i < list_size(fm->bloques); i++)
    {
        sprintf(bloques, "%s,%d", bloques, (int)(intptr_t)list_get(fm->bloques, i));
    }

    return strcat(bloques, "]");
}

int escuchar_conexiones()
{

    char *port = config_get_string_value(config, "PUERTO");
    char *ip = "127.0.0.1";

    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
    hints.ai_socktype = SOCK_STREAM; // Indica que usaremos el protocolo TCP

    getaddrinfo(ip, port, &hints, &server_info); // Carga en server_info los datos de la conexion

    int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if (server_socket < 0)
    {
        _exit_with_error(-1, "No creo el socket", NULL);
        exit_gracefully(1);
    }

    int retorno = bind(server_socket, server_info->ai_addr, server_info->ai_addrlen);

    freeaddrinfo(server_info); // No lo necesitamos mas

    if (retorno < 0)
    {
        _exit_with_error(server_socket, "No logro el bind", NULL);
        exit_gracefully(1);
    }

    log_info(logger, "Empiezo a escuchar el socket");

    int conexion = listen(server_socket, 1);

    if (conexion < 0)
    {
        _exit_with_error(server_socket, "Fallo el listen", NULL);
        exit_gracefully(1);
    }

    //int conexion_aceptada = accept(server_socket, server_info->ai_addr, server_info->ai_addrlen);
    int conexion_aceptada = accept(server_socket, NULL, NULL);

    if (conexion_aceptada < 0)
    {
        _exit_with_error(server_socket, "Fallo el accept", NULL);
        exit_gracefully(1);
    }

    log_info(logger, "Conexion aceptada nuevo socket: %d", conexion_aceptada);

    close(server_socket);
    return conexion_aceptada;
}

void borrar_archivo(Paquete *paquete)
{

    int offset = 0;
    char *ruta = string_deserializar(paquete->Payload, &offset);

	int existe = validar_archivo(paquete, file_path);

	if (!existe)
		enviar_error(ARCHIVO_NO_EXISTE);

    //TODO: Lee metadata del archivo y devuelve los bloques
    t_config *metadata = config_create(ruta);
    char **bloques_ocupados = config_get_array_value(metadata, "BLOQUES");

    int cantidad_bloques_del_path = config_get_int_value(metadata, "TAMANIO") / tamanio_bloques;

    for (int i = 0; i < cantidad_bloques_del_path; i++)
    {
        //TODO: Borra los .bin
        //remove();
    }

    //TODO: Modifica el bitarray
    //liberar();
    config_destroy(metadata);
    //Borra los metadata
    remove(ruta);
    //Crea paquete OK
    Paquete respuesta;
    respuesta.header = cargar_header(0, SUCCESS, MDJ);
    EnviarPaquete(socket_dam, &respuesta);
}

char *leer_bitmap()
{
    char *bitmap_bin = "/Metadata/Bitmap.bin";
    char *bitmap_path = malloc(strlen(bitmap_bin) + strlen(mnt_path) + 1);
    sprintf(bitmap_path, "%s%s", mnt_path, bitmap_bin);

    FILE *f = fopen(bitmap_path, "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *bitmap = malloc(size + 1);
    fread(bitmap, size, 1, f);
    fclose(f);

    bitmap[size] = '\0';
    return bitmap;
}

void cargar_metadata()
{
    char *metadata_bin = "/Metadata/Metadata.bin";
    char *metadata_path = malloc(strlen(metadata_bin) + strlen(mnt_path) + 1);
    sprintf(metadata_path, "%s%s", mnt_path, metadata_bin);
    t_config *bitarray_metadata = config_create(metadata_path);
    tamanio_bloques = config_get_int_value(bitarray_metadata, "TAMANIO_BLOQUES");
    cantidad_bloques = config_get_int_value(bitarray_metadata, "CANTIDAD_BLOQUES");
    config_destroy(bitarray_metadata);
}

void cargar_bitarray()
{
    cargar_metadata();

    char *bitmap = leer_bitmap();
    bitarray = bitarray_create_with_mode(bitmap, cantidad_bloques / 8, LSB_FIRST);
    size_t n = bitarray_get_max_bit(bitarray);
    log_info(logger, "El bitarray tiene %d bits", n);
}

#endif
