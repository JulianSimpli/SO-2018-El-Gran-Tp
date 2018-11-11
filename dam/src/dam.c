#include "dam.h"

void *mensajes_mdj();
void *mensajes_safa();
void *aceptar_cpu(int);
void *levantar_hilo(void *);
void *interpretar_mensajes_de_cpu(void *);
void *interpretar_mensajes_de_mdj(void *);
void *interpretar_mensajes_de_safa(void *);
void *interpretar_mensajes_de_fm9(void *);
int crear_socket_fm9();

Mensaje *recibir_mensaje(int socket);

int socket_safa;
int socket_mdj;
int socket_fm9;
int transfer_size;

int main(int argc, char **argv)
{
	inicializar(argv);

	int maximas_conexiones = config_get_int_value(config, "MAXIMAS_CONEXIONES");

	//Crea por unica vez los sockets con los que tiene conexiones permanentes
	//Primero hago el handshake con fm9 para poder tener el tamanio de linea
	socket_safa = handshake_safa();
	socket_mdj = handshake_mdj();
	socket_fm9 = handshake_fm9();

	//inicializamos el semaforo en maximas_conexiones - 3 que son fijas
	aceptar_cpus();

	levantar_hilo(interpretar_mensajes_de_safa);
	levantar_hilo(interpretar_mensajes_de_mdj);
	levantar_hilo(interpretar_mensajes_de_fm9);
	return 0;
}

void aceptar_cpus()
{
	int socket_ligado = ligar_socket();
	//sem_init(&sem_maximas_conextiones, 0, 0); el tercer valor va ser maximas conexiones - 3
	//sem_post cuando se libera una conexion de cpu recv devuelve 0 o -1 en caso de error

	for (;;)
	{
		//sem_wait
		int conexion_aceptada = accept(socket_ligado, NULL, NULL);

		if (conexion_aceptada < 0)
		{
			_exit_with_error(socket_ligado, "Fallo el accept", NULL);
			exit_gracefully(1);
		}

		log_info(logger, "Acepto cpu en socket: %d", conexion_aceptada);

		levantar_hilo(interpretar_mensajes_de_cpu);
	}
}

/**
 * Se encarga de crear el socket y mandar el primer mensaje
 * Devuelve el socket 
 */
int handshake_mdj()
{
	int mdj = crear_socket_mdj();
	handshake(mdj, ELDIEGO);

	Mensaje *mensaje = recibir_mensaje(mdj);
	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE)
	{
		_exit_with_error(mdj, "No se logro el handshake", NULL);
	}
	log_info(logger, "Se concreto el handshake con MDJ, empiezo a recibir mensajes");
	return mdj;
}

/**
 * Se encarga de crear el socket, mandar el primer mensaje
 * Recibir de fm9 el tamanio de linea que tiene el sistema
 * Devuelve el socket 
 */
int handshake_fm9()
{
	int fm9 = crear_socket_fm9();
	handshake(fm9, ELDIEGO);
	Mensaje *mensaje = recibir_mensaje(fm9);
	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE)
	{
		_exit_with_error(fm9, "No se logro el primer paso de datos", NULL);
	}
	//Guardo en la global tamanio de linea que tiene fm9
	memcpy(&tamanio_linea, mensaje->paquete.Payload, sizeof(u_int32_t));
	log_info(logger, "Se concreto el handshake con FM9, me pasa el tamanio de linea %d", tamanio_linea);
	free(mensaje->paquete.Payload);
	free(mensaje);
	return fm9;
}

/**
 * Se encarga de crear el socket, mandar el primer mensaje
 * Devuelve el socket 
 */
int handshake_safa()
{
	int safa = crear_socket_safa();
	handshake(safa, ELDIEGO);
	Mensaje *mensaje = recibir_mensaje(safa);
	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE)
	{
		_exit_with_error(safa, "No se logro el primer paso de datos", NULL);
	}
	//Guardo en la global tamanio de linea que tiene safa
	memcpy(&tamanio_linea, mensaje->paquete.Payload, sizeof(u_int32_t));
	log_info(logger, "Se concreto el handshake con safa, me pasa el tamanio de linea %d", tamanio_linea);
	free(mensaje->paquete.Payload);
	free(mensaje);
	return safa;
}

// void * mensajes_mdj()
// 	int socket = crear_socket_mdj();

// 	log_info(logger, "Voy a enviar mensaje");
// 	Mensaje mensaje = crear_handshake_mdj(socket);
// 	enviar_mensaje(mensaje);
// 	Mensaje *respuesta = recibir_mensaje(socket);
// 	log_info(logger, "Recibido");
// }

int crear_socket_safa()
{
	char *puerto_safa = config_get_string_value(config, "PUERTO_SAFA");

	char *ip_safa = config_get_string_value(config, "IP_SAFA");

	int socket = connect_to_server(ip_safa, puerto_safa);

	return socket;
}

int crear_socket_fm9()
{
	char *puerto_fm9 = config_get_string_value(config, "PUERTO_FM9");

	char *ip_fm9 = config_get_string_value(config, "IP_FM9");

	int socket = connect_to_server(ip_fm9, puerto_fm9);

	return socket;
}

void *interpretar_mensajes_de_mdj(void *args)
{
	Paquete *paquete = recibir_mensaje(socket_mdj);

	//interpreta
	Tipo accion = mensaje->paquete.header.tipoMensaje;
	switch (accion)
	{
	case VALIDAR_ARCHIVO:
		break;
	default:
		log_warning(logger, "No se pudo interpretar el mensaje enviado por MDJ");
		break;
	}
	//enviar_mensaje
}

/**
 * Devuelve <= 0 si murio el socket
 * al recibir solo el header o todo el paquete 
 */
int recibir_todo(Paquete *paquete, int socketFD, uint32_t cantARecibir)
{
	int ret = recibir_partes(paquete, socketFD, sizeof(Header));
	if (ret <= 0) 
		return ret;
	paquete->Payload = malloc(paquete->header.tamPayload);
	ret = recibir_partes(paquete->Payload, socketFD, paquete->header.tamPayload);
	return ret;
}

int recibir_partes(void *paquete, int socketFD, u_int32_t cant_a_recibir)
{

	void *datos = malloc(cant_a_recibir);
	int recibido = 0;
	int totalRecibido = 0;
	int len = transfer_size;

	while (totalRecibido != cant_a_recibir)
	{
		if (cant_a_recibir < transfer_size) {
			len = cant_a_recibir;
		}

		recibido = recv(socketFD, datos + totalRecibido, len, 0);

		//man recv en el caso de -1 es error pero tambien lo matamos
		if (recibido <= 0)
		{
			printf("Cliente Desconectado\n");
			//TODO: Preguntar que socket se desconecto, si no es CPU o si es el ultimo CPU tiene que morir todo.
			close(socketFD); // ¡Hasta luego!
			//sem_post
			break;
		}

		totalRecibido += recibido;
	}

	memcpy(paquete, datos, cant_a_recibir);
	free(datos);

	return recibido;
}

void *interpretar_mensajes_de_safa(void *args)
{

}

void *interpretar_mensajes_de_fm9(void *args)
{
	int recibido =
}

void leer_config(char *path)
{
	config = config_create(strcat(path, "/DAM.config"));
}

void inicializar_log(char *program)
{
	logger = log_create("DAM.log", program, true, LOG_LEVEL_DEBUG);
}

void inicializar(char **argv)
{
	inicializar_log(argv[0]);
	log_info(logger, "Logger cargado exitosamente");
	leer_config(argv[1]);
	log_info(logger, "Config cargada exitosamente");
	transfer_size = config_get_int_value("TRANSFER_SIZE");
}

int escuchar_conexiones(int server_socket)
{

	log_info(logger, "Empiezo a escuchar el socket %d", server_socket);

	int conexion_aceptada = accept(server_socket, NULL, NULL);

	if (conexion_aceptada < 0)
	{
		_exit_with_error(server_socket, "Fallo el accept", NULL);
		exit_gracefully(1);
	}

	log_info(logger, "Conexion aceptada nuevo socket: %d", conexion_aceptada);

	return conexion_aceptada;
}

void *levantar_hilo(void *funcion)
{
	pthread_t p_thread;
	pthread_create(&p_thread, NULL, funcion, NULL);
}

/**
 * Recibe el mensaje en el socket, interpreta que es lo que require
 * Genera la respuesta y muere el hilo al enviarla
 * El proximo mensaje que se reciba de CPU va aparecer en el select del hilo principal
 * Y luego generara uno nuevo para que lo atiende otra vez 
 */
void *interpretar_mensajes_de_cpu(void *args)
{
	//For para recibir mensaje de cpu, interpretarlo y enviar la respuesta
	log_info(logger, "Soy el hilo %d, recibi una conexion en el socket: %d", process_get_thread_id(), ((Mensaje *)args)->socket);
	log_info(logger, "Interpreto mensaje");
}
