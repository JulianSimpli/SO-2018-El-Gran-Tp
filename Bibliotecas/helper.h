#ifndef HELPER_
#define HELPER_
#define LAMBDA(c_) ({ c_ _;}) //Para funciones lambda
#define INTSIZE sizeof(u_int32_t)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <commons/temporal.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/txt.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <ctype.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include "dtb.h"

// Estos define es lo que hace stdbool.h
#define true 1
#define false 0

t_log *logger;
t_config *config;

typedef struct Posicion {
    u_int32_t pid;
    u_int32_t segmento;
    u_int32_t pagina;
    u_int32_t offset;
} __attribute__((packed)) Posicion;

//para guardar funciones y estructuras que se necesiten

char* integer_to_string(char*string,int x);
size_t getFileSize(const char* filename);
void *string_serializar(char *string, int *desplazamiento);
char *string_deserializar(void *data, int *desplazamiento);
void *serializar_pid_y_pc(u_int32_t pid, u_int32_t pc, int *tam_pid_y_pc);
void deserializar_pid_y_pc(void *payload, u_int32_t *pid, u_int32_t *pc, int *desplazamiento);
void *serializar_posicion(Posicion *posicion, int *tam_posicion);
Posicion *deserializar_posicion(void *payload, int *desplazamiento);
Posicion *generar_posicion(DTB *dtb, ArchivoAbierto *archivo, u_int32_t offset);
void _exit_with_error(int socket, char *error_msg, void *buffer);
void exit_gracefully(int return_nr);

#endif /* HELPER_*/
