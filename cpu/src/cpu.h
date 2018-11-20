#ifndef CLIENTE_H_
#define CLIENTE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/error.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <string.h>
#include "../../Bibliotecas/sockets.h"
#include "primitivas.h"
#include "../../Bibliotecas/dtb.h"

//Logger y config globales
t_log *logger;
t_config *config;

//Use this for simple validation of the mdj config file keys
int keys = 3;
int retardo;

/* The names of functions that actually do the manipulation. */

void com_help(char *linea);
void leer_config(char *path);
void inicializar_log(char *program);
void inicializar(char **argv);
void config_reload();
int inicializar_socket();
int connect_to_server(char *ip, char *port);
void handshake(int, int);
int interpretar(char *linea);

void exit_gracefully(int return_nr);
void _exit_with_error(int socket, char *error_msg, void *buffer);
void enviar_mensaje(Mensaje mensaje);
Mensaje *recibir_mensaje(int conexion);
int crear_socket_safa();

#endif
