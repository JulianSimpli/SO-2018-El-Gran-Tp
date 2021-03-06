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
#include <commons/process.h>
#include "../../Bibliotecas/sockets.h"
#include "../../Bibliotecas/dtb.h"
#include "../../Bibliotecas/helper.h"
#include "../../Bibliotecas/loggers.h"


//Logger y config globales

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
void handshake_safa();
void handshake_dam();
void handshake_fm9();
int interpretar(char *linea);
void bloquea_dummy(Paquete *paquete);
int bloqueate_safa(DTB *dtb, Tipo tipo_mensaje);
int enviar_pid_y_pc(DTB *dtb, Tipo tipo_mensaje);
void dtb_liberar(DTB *dtb);

void enviar_mensaje(Mensaje mensaje);
Mensaje *recibir_mensaje(int conexion);
int crear_socket_safa();
int crear_socket_dam();
int crear_socket_fm9();

//Primitivas
int ejecutar_abrir(char **, DTB*);
int ejecutar_concentrar(char **,DTB*);
int ejecutar_asignar(char **,DTB*);
int ejecutar_wait(char **,DTB*);
int ejecutar_signal(char **,DTB*);
int ejecutar_flush(char **,DTB*);
int ejecutar_close(char **,DTB*);
int ejecutar_crear(char **,DTB*);
int ejecutar_borrar(char **,DTB*);

/* A structure which contains information on the commands this program
   can understand. */
typedef int (*DoRunTimeChecks)();

struct Primitiva
{
	char *name;			  /* User printable name of the function. */
	DoRunTimeChecks func; /* Function to call to do the job. */
	char *doc;			  /* Documentation for this function.  */
};

struct Primitiva primitivas[] = {
	{"abrir", ejecutar_abrir, "El comando abrir permitirá abrir un nuevo archivo para el escriptorio."},
	{"concentrar", ejecutar_concentrar, "La primitiva concentrar será un comando de no operación. El objetivo del mismo es hacer correr una unidad de tiempo de quantum."},
	{"asignar", ejecutar_asignar, "La primitiva asignar permitirá la asignación de datos a una línea dentro de path pasado."},
	{"wait", ejecutar_wait, "La primitiva Wait permitirá la espera y retención de distintos recursos tanto existentes como no que administrá S-AFA."},
	{"signal", ejecutar_signal, "La primitiva Signal permitirá la liberación de un recurso que son administrados S-AFA."},
	{"flush", ejecutar_flush, "La primitiva Flush permite guardar el contenido de FM9 a MDJ."},
	{"close", ejecutar_close, "La primitiva Close permite liberar un archivo abierto que posea el G.DT."},
	{"crear", ejecutar_crear, "La primitiva crear permitirá crear un nuevo archivo en el MDJ. Para esto, se deberá utilizar de intermediario a El Diego."},
	{"borrar", ejecutar_borrar, "La primitiva borrar permitirá eliminar un archivo de MDJ."},
};

#endif
