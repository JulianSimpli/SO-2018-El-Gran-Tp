#ifndef DAM_H_
#define DAM_H_

#include <stdio.h> // Por dependencia de readline en algunas distros de linux :)
#include <string.h>
#include <stdlib.h> // Para malloc
#include <sys/socket.h> // Para crear sockets, enviar, recibir, etc
#include <netdb.h> // Para getaddrinfo
#include <unistd.h> // Para close
#include <readline/readline.h> // Para usar readline
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <errno.h>
#include <pthread.h>
#include <commons/process.h>
#include "../../Bibliotecas/sockets.h"

// Definimos algunas variables globales
t_log * logger;
t_config* config;
int tamanio_linea;
int transfer_size;

#define IP "127.0.0.1"
#define PUERTO "8080"

// Comunes de carga
void leer_config();
void inicializar_log(char *program);
void inicializar(char **argv);
void handshake(int, int);
int escuchar_conexiones();
int crear_socket_mdj();
int ligar_socket();
int recibir_todo(Paquete *paquete, int socketFD, uint32_t cantARecibir);
int recibir_partes(void *paquete, int socketFD, u_int32_t cant_a_recibir);

// Finalmente, los prototipos de las funciones que vamos a implementar
void configure_logger();
int  connect_to_server(char*, char*);
void _exit_with_error(int , char* , void * );
void exit_gracefully(int return_nr);

void handshake_fm9();
void handshake_mdj();
void handshake_safa();
void handshake_cpu(int socket);
int crear_socket_safa();

int connect_to_server(char * ip, char * port) {
  struct addrinfo hints;
  struct addrinfo *server_info;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
  hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

  getaddrinfo(ip, port, &hints, &server_info);  // Carga en server_info los datos de la conexion

  int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

  if(server_socket < 0){
	  perror("Error:");
  	  _exit_with_error(-1,"No se pudo crear el socket",NULL);
  }

  int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

  freeaddrinfo(server_info);  // No lo necesitamos mas

  if(retorno < 0){
	  _exit_with_error(server_socket,"No me pude conectar al servidor",NULL);
  }

  log_info(logger, "Conectado!");

  return server_socket;
}

/**
 * Busca en su config el socket que tiene que abrir
 * para escuchar a los cpus que se quieran conectar 
 */
int ligar_socket(){

	char * port = config_get_string_value(config, "PUERTO");
	char * ip =  "127.0.0.1";

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	getaddrinfo(ip, port, &hints, &server_info); // Carga en server_info los datos de la conexion

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if(server_socket < 0){
		_exit_with_error(-1, "No creo el socket", NULL);
		exit_gracefully(1);
	}

	int retorno = bind(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info); // No lo necesitamos mas

	if (retorno < 0) {
		_exit_with_error(server_socket, "No logro el bind", NULL);
		exit_gracefully(1);
	}

	int conexion = listen(server_socket,3);

	if(conexion < 0){
		_exit_with_error(server_socket, "Fallo el listen", NULL);
		exit_gracefully(1);
	}

	return server_socket;
}

int crear_socket_mdj() {
	char * puerto_mdj = config_get_string_value(config,"PUERTO_MDJ");

	char * ip_mdj = config_get_string_value(config, "IP_MDJ");

	return connect_to_server(ip_mdj,puerto_mdj);
}

void _exit_with_error(int socket, char* error_msg, void * buffer) {
  if (buffer != NULL) {
    free(buffer);
  }
  log_error(logger, error_msg);
  close(socket);
  exit_gracefully(1);
}

void exit_gracefully(int return_nr) {
  log_destroy(logger);
  exit(return_nr);
}

#endif /* DAM_H_ */
