#include "cpu.h"
#include <commons/string.h>

int main(int argc, char **argv)
{
  inicializar(argv);

  //TODO: Handshake con SAFA
  //TODO: Recibe quantum por primera vez de SAFA
  //TODO: Averiguar si tiene que estar escuchando a un mensaje de cambio de quantum o algoritmo de SAFA

  //TODO: Handshake con el DIEGO

  //TODO: Handshake con FM9

  //TODO: Listen SAFA
  //TODO: Recibir DTB de SAFA
  //TODO: Verificar si flag esta en 0 hacer operacion dummy
  //TODO: Operacion dummy
  //TODO: Flag en 1 entro a ejecutar primitivas parseando y desalojo cuando me quedo sin quantum
  char *ip_diego = config_get_string_value(config, "IP_DIEGO");
  char *puerto_diego = config_get_string_value(config, "PUERTO_DIEGO");

  int diego = connect_to_server(ip_diego, puerto_diego);

  handshake(diego, CPU);

  return 0;
}

/**
 * Se encarga de crear el socket y mandar el primer mensaje
 * Devuelve el socket 
 */
int handshake_safa()
{
	int safa = crear_socket_safa();
	handshake(safa, CPU);
	Mensaje *mensaje = recibir_mensaje(safa);
	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE) {
		_exit_with_error(safa, "No se logro el handshake", NULL);
	}
	log_info(logger, "Se concreto el handshake con MDJ, empiezo a recibir mensajes");
	return safa;
}

void handshake(int socket, int emisor) {
	Mensaje *mensaje = malloc(sizeof(Mensaje));
	mensaje->socket = socket;
	mensaje->paquete.header.tipoMensaje = ESHANDSHAKE;
	mensaje->paquete.header.tamPayload = 0;
	mensaje->paquete.header.emisor = emisor;
  enviar_mensaje(*mensaje);
}

void leer_config(char *path)
{
  config = config_create(strcat(path, "/CPU.config"));
}

void inicializar_log(char *program)
{
  logger = log_create("CPU.log", program, true, LOG_LEVEL_DEBUG);
}

void inicializar(char **argv)
{
  inicializar_log(argv[0]);
  log_info(logger, "Log cargado exitosamente");
  leer_config(argv[1]);
  log_info(logger, "Config cargada exitosamente");
}

void config_reload()
{
  retardo = config_get_int_value(config, "RETARDO") / 1000;
}

int inicializar_socket()
{
  int puerto = config_get_int_value(config, "PUERTO");
}

int connect_to_server(char *ip, char *port)
{
  struct addrinfo hints;
  struct addrinfo *server_info;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;     // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
  hints.ai_socktype = SOCK_STREAM; // Indica que usaremos el protocolo TCP

  getaddrinfo(ip, port, &hints, &server_info); // Carga en server_info los datos de la conexion

  int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

  if (server_socket < 0)
  {
    perror("socket: ");
    _exit_with_error(-1, "No se pudo crear el socket", NULL);
  }

  int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

  freeaddrinfo(server_info); // No lo necesitamos mas

  if (retorno < 0)
  {
    perror("connect: ");
    _exit_with_error(server_socket, "No me pude conectar al servidor", NULL);
  }

  log_info(logger, "Conectado!");

  return server_socket;
}

int interpretar(char *linea)
{
	int i = 0, existe = 0;
	//Calcula la cantidad de primitivas que hay en el array
	size_t cantidad_primitivas = sizeof(primitivas) / sizeof(primitivas[0]) - 1;

	//Para poder identificar el comando y que el resto son parametros necesarios para que ejecute
	//Por ej: linea = abrir /home/utnso/
	char **parameters = string_split(linea, " ");

	for (i; i < cantidad_primitivas; i++)
	{
		if (!strcmp(primitivas[i].name, parameters[0]))
		{
			existe = 1;
			log_info(logger, primitivas[i].doc);
			//llama a la funcion que tiene guardado esa primitiva en la estructura
			primitivas[i].func(parameters);
			break;
		}
	}

	if (!existe)
	{
		error_show("No existe el comando %s\n", linea);
	}

	return existe;
}

void enviar_mensaje(Mensaje mensaje)
{
	void *buffer = malloc(sizeof(Paquete));

	int mensaje_enviado = send(mensaje.socket, buffer, sizeof(Paquete), 0);

	if (mensaje_enviado < 0)
	{
		log_error(logger, "No pudo enviar el mensaje");
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

int crear_socket_safa()
{
	char *puerto_safa = config_get_string_value(config, "PUERTO_SAFA");

	char *ip_safa = config_get_string_value(config, "IP_SAFA");

	int socket = connect_to_server(ip_safa, puerto_safa);

	return socket;
}

void _exit_with_error(int socket, char *error_msg, void *buffer)
{
  if (buffer != NULL)
    free(buffer);

  log_error(logger, error_msg);
  close(socket);
  exit_gracefully(1);
}

void exit_gracefully(int return_nr)
{
  if (return_nr == 1)
    perror("Error: ");

  log_destroy(logger);
  exit(return_nr);
}
