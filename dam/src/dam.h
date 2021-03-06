#ifndef DAM_H_
#define DAM_H_

#include <stdio.h> // Por dependencia de readline en algunas distros de linux :)
#include <string.h>
#include <stdlib.h>			   // Para malloc
#include <sys/socket.h>		   // Para crear sockets, enviar, recibir, etc
#include <netdb.h>			   // Para getaddrinfo
#include <unistd.h>			   // Para close
#include <readline/readline.h> // Para usar readline
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <errno.h>
#include <pthread.h>
#include <commons/process.h>
#include "../../Bibliotecas/loggers.h"
#include "../../Bibliotecas/sockets.h"
#include "../../Bibliotecas/dtb.h"

// Definimos algunas variables globales
t_config *config;
int tamanio_linea;
sem_t sem_mdj;
sem_t sem_fm9;
sem_t sem_safa;

// Comunes de carga
void leer_config();
void inicializar_log(char *program);
void inicializar(char **argv);
void inicializar_semaforos();
void handshake(int, int);
int escuchar_conexiones();
int crear_socket_mdj();
int ligar_socket();

// Finalmente, los prototipos de las funciones que vamos a implementar
void configure_logger();
int connect_to_server(char *, char *);
void _exit_with_error(int, char *, void *);
void exit_gracefully(int return_nr);

void handshake_fm9();
void handshake_mdj();
void handshake_safa();
void handshake_cpu(int socket);
int crear_socket_safa();
int crear_socket_fm9();

void *mensajes_mdj();
void *mensajes_safa();
void *aceptar_cpu(int);
pthread_t levantar_hilo(void *);

void *interpretar_mensajes_de_cpu(void *);
void *interpretar_mensajes_de_mdj(void *);
void *interpretar_mensajes_de_safa(void *);
void *interpretar_mensajes_de_fm9(void *);
void aceptar_cpus();

void enviar_obtener_datos(char *path, int size);
void enviar_guardar_datos(Paquete *paquete);

//Interpreta cpu
void abrir(Paquete *paquete);
void es_dtb_dummy(Paquete *paquete);
void crear_archivo(Paquete *paquete);
void borrar_archivo(Paquete *paquete);
void flush(Paquete *paquete);

int socket_safa;
int socket_mdj;
int socket_fm9;
int transfer_size;

//Funciones

int connect_to_server(char *ip, char *port)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	 // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM; // Indica que usaremos el protocolo TCP

	getaddrinfo(ip, port, &hints, &server_info); // Carga en server_info los datos de la conexion

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if (server_socket < 0)
	{
		perror("Error:");
		_exit_with_error(-1, "No se pudo crear el socket", NULL);
	}

	int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info); // No lo necesitamos mas

	if (retorno < 0)
	{
		_exit_with_error(server_socket, "No me pude conectar al servidor", NULL);
	}

	log_info(logger, "Conectado!");

	return server_socket;
}

/**
 * Busca en su config el socket que tiene que abrir
 * para escuchar a los cpus que se quieran conectar 
 */
int ligar_socket()
{

	char *port = config_get_string_value(config, "PUERTO");
	char *ip = config_get_string_value(config, "IP");

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	 // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
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

	int conexion = listen(server_socket, 3);

	if (conexion < 0)
	{
		_exit_with_error(server_socket, "Fallo el listen", NULL);
		exit_gracefully(1);
	}

	return server_socket;
}

void enviar_paquete(int socket, Paquete *paquete)
{
	int total = paquete->header.tamPayload;
	int desplazamiento = 0;
	int enviar = transfer_size;
		
	int enviado = send(socket, &paquete->header, TAMANIOHEADER, 0);
	log_header(logger, paquete, "Envie header:");

	if (enviado == -1)
		_exit_with_error(socket, "No pudo enviar el header", paquete);

	usleep(1*1000);

	void *buffer = malloc(paquete->header.tamPayload);
	memcpy(buffer, paquete->Payload, paquete->header.tamPayload);

	while (desplazamiento < total)
	{
		if ((total - desplazamiento) < transfer_size)
			enviar = total - desplazamiento;

		//int enviado = send(socket, paquete->Payload + desplazamiento, enviar, 0);
		int enviado = send(socket, buffer + desplazamiento, enviar, 0);

		if (enviado == -1)
			_exit_with_error(socket, "No pudo enviar el paquete", paquete);

		desplazamiento += enviado;
		//log_debug(logger, "Envie %d/%d", enviar, total);
	}
	log_debug(logger, "Envie %i bytes", total);
}

#endif /* DAM_H_ */
