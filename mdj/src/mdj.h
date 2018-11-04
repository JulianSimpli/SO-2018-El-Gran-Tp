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

typedef struct archivo_metadata
{
  char *nombre;
  int tamanio;
  t_list *bloques;
} Archivo_metadata;

void leer_config(char *path);
void inicializar_log(char *program);
void inicializar(char **argv);
void retardo();
int interpretar(char *linea);
int escuchar_conexiones();
Mensaje *recibir_mensaje(int conexion);
void *atender_dam();
void *atender_peticion(void *);
Mensaje *interpretar_mensaje(Mensaje *);
void handshake(int, int);
void enviar_mensaje(Mensaje);
void marcarBitarray(t_config *metadata);
int create_block(int index, int cantidad_bytes);
t_list *get_space(int size);
char *convertir_bloques_a_string(Archivo_metadata *fm);
char *mnt_path;
char *file_path;
char *blocks_path;
char *metadata_path;
int cantidad_bloques;
int tamanio_bloques;

void _exit_with_error(int socket, char *error_msg, void *buffer);
void exit_gracefully(int return_nr);
int validar_archivo(Mensaje *mensaje, char *);
void crear_archivo(Mensaje *mensaje);
void obtener_datos(Mensaje *mensaje);
void guardar_datos(Mensaje *mensaje);

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

void handshake(int socket, int emisor)
{
  Mensaje *mensaje = malloc(sizeof(Mensaje));
  mensaje->socket = socket;
  mensaje->paquete.header.tipoMensaje = ESHANDSHAKE;
  mensaje->paquete.header.tamPayload = 0;
  mensaje->paquete.header.emisor = emisor;
  enviar_mensaje(*mensaje);
}

void com_list(char *linea)
{
  //fijarse en que path está y mostrar que es directorio y que es archivo
  printf("The folder has this currents files: \n");
  system("ls -la");
}

void com_cat(char *linea)
{
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

  bitarray_set_bit(bitarray, index);

  return create_block_file(block_path, cantidad_bytes) != 0;
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

void _exit_with_error(int socket, char *error_msg, void *buffer)
{
  if (buffer != NULL)
  {
    free(buffer);
  }
  log_error(logger, error_msg);
  close(socket);
  exit_gracefully(1);
}

void exit_gracefully(int return_nr)
{
  if (return_nr == 1)
  {
    perror("Error: ");
  }
  bitarray_destroy(bitarray);
  config_destroy(config);
  log_destroy(logger);
  exit(return_nr);
}

#endif