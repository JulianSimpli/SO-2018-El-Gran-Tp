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
#include "../../Bibliotecas/dtb.h"

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
void recibir_paquete(int socket, Paquete *paquete);
void recibir_partes(int socket, Paquete *paquete, int cant_a_recibir);

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

void enviar_paquete(int socket, Paquete *paquete) {
	int total = TAMANIOHEADER + paquete->header.tamPayload;
	int desplazamiento = 0;
	int enviar = transfer_size;

	while (desplazamiento != total) {
		if ((total - desplazamiento) < transfer_size)
			enviar = total - desplazamiento;	

		int enviado = send(socket, paquete->Payload + desplazamiento, enviar, 0);
		
		if (enviado == -1)
			_exit_with_error(socket, "No pudo enviar el paquete", paquete);

		desplazamiento += enviado;
	}
}

void recibir_paquete(int socket, Paquete *paquete)
{
	recibir_partes(socket, paquete, TAMANIOHEADER);
	paquete->Payload = malloc(paquete->header.tamPayload);
	recibir_partes(socket, paquete->Payload, paquete->header.tamPayload);
}

void recibir_partes(int socket, Paquete *paquete, int cant_a_recibir)
{
	void *datos = malloc(cant_a_recibir);
	int total_recibido = 0;
	int len = transfer_size;

	while (total_recibido != cant_a_recibir)
	{
		if (cant_a_recibir < transfer_size)
			len = cant_a_recibir;

		int recibido = recv(socket, datos + total_recibido, len, 0);

		//man recv en el caso de -1 es error pero tambien lo matamos
		if (recibido <= 0)
			_exit_with_error(socket, "No pudo recibir el paquete", datos);

		total_recibido += recibido;
	}

	memcpy(paquete, datos, cant_a_recibir);
	free(datos);
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
