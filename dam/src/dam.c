#include "dam.h"

void *mensajes_mdj();
void *mensajes_safa();
void *aceptar_cpu(int);
void *aceptar_proceso(void*, Mensaje*);
void *interpretar_mensajes_de_cpu(void*);
void *interpretar_mensajes_de_mdj(void*);
void *interpretar_mensajes_de_safa(void*);
void *interpretar_mensajes_de_fm9(void*);
int crear_socket_fm9();

Mensaje * recibir_mensaje(int socket);

int main(int argc,char ** argv ) {
	inicializar(argv);
	
	int sret, ret;
	int nfds = config_get_int_value(config, "MAXIMAS_CONEXIONES");

	//Crea por unica vez los sockets con los que tiene conexiones permanentes
	//Primero hago el handshake con fm9 para poder tener el tamanio de linea
	//int fm9 = handshake_fm9();
	int mdj = handshake_mdj();
	//int safa = handshake_safa();

	//Abre su socket
	int socket_ligado = ligar_socket();
	fd_set lecturas;
	fd_set master;
	FD_ZERO(&master);
	
	FD_SET(socket_ligado, &master);
	
	for(;;) {
		//FD_SET(fm9, &lecturas);
		FD_SET(mdj, &lecturas);
		//FD_SET(safa, &lecturas);
		FD_SET(socket_ligado, &lecturas);
		lecturas = master;

		sret = select(nfds+1, &lecturas, NULL, NULL, NULL);
		log_info(logger, "sret: %d ", sret);
		if (sret == -1) {
			perror("select()");
		} else if(sret == 0) {
			log_warning(logger, "TIMEOUT");
		}

		if (FD_ISSET(socket_ligado, &lecturas)) {
			
			log_info(logger, "Voy a aceptar lo que escuche en el socket %d", socket_ligado);

			int conexion_aceptada = accept(socket_ligado, NULL, NULL);

			if(conexion_aceptada < 0){
				_exit_with_error(socket_ligado, "Fallo el accept", NULL);
				exit_gracefully(1);
			}

			log_info(logger, "Conexion aceptada nuevo socket: %d", conexion_aceptada);

			//Recibir el mensaje del socket
			Mensaje *mensaje = recibir_mensaje(conexion_aceptada);
			int emisor = mensaje->paquete.header.emisor;

			//Interpreta el content header para saber quien es el emisor
			//Abrir un hilo para atender al emisor
			//El hilo es el que se encarga y no esperamos que termine			
			switch(emisor) {
				case CPU: 
					log_info(logger, "Se conecto un CPU");
					aceptar_proceso(interpretar_mensajes_de_cpu, mensaje);
					break;
				case FM9: 
					log_info(logger, "Se conecto FM9");
					aceptar_proceso(interpretar_mensajes_de_fm9, mensaje);
					break;
				case MDJ: 
					log_info(logger, "Se conecto MDJ");
					aceptar_proceso(interpretar_mensajes_de_mdj, mensaje);
					break;
				case SAFA: 
					log_info(logger, "Se conecto SAFA");
					aceptar_proceso(interpretar_mensajes_de_safa, mensaje);
					break;
				default:
					log_warning(logger, "No se pudo identificar al emisor");
					break;
			}
			
		}
		
	}
	return EXIT_SUCCESS;
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
	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE) {
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
	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE) {
		_exit_with_error(fm9, "No se logro el primer paso de datos", NULL);
	}
	//Guardo en la global tamanio de linea que tiene fm9
	tamanio_linea = (intptr_t) mensaje->paquete.Payload;
	log_info(logger, "Se concreto el handshake con FM9, me pasa el tamanio de linea %d", tamanio_linea);
	free(mensaje);
	free(mensaje);
	return fm9;
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
	char * puerto_safa = config_get_string_value(config,"PUERTO_SAFA");

	char * ip_safa = config_get_string_value(config, "IP_SAFA");

	int socket = connect_to_server(ip_safa,puerto_safa);

	return socket;
}

int crear_socket_fm9()
{
	char * puerto_fm9 = config_get_string_value(config,"PUERTO_FM9");

	char * ip_fm9 = config_get_string_value(config, "IP_FM9");

	int socket = connect_to_server(ip_fm9, puerto_fm9);

	return socket;
}

void * interpretar_mensajes_de_mdj(void * args)
{
	//interpreta
	Tipo accion = ((Mensaje*)args)->paquete.header.tipoMensaje;
	switch(accion) {
		case VALIDAR_ARCHIVO:
			break;
		default:
			log_warning(logger, "No se pudo interpretar el mensaje enviado por MDJ");
			break;
	}
	//enviar_mensaje
}

void * interpretar_mensajes_de_safa(void * args)
{

}

void * interpretar_mensajes_de_fm9(void * args)
{

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
   log_info(logger,"Logger cargado exitosamente");
   leer_config(argv[1]);
   log_info(logger,"Config cargada exitosamente");
}

int escuchar_conexiones(int server_socket) {

	log_info(logger, "Empiezo a escuchar el socket %d", server_socket);

	int conexion_aceptada = accept(server_socket, NULL, NULL);

	if(conexion_aceptada < 0){
		_exit_with_error(server_socket, "Fallo el accept", NULL);
		exit_gracefully(1);
	}

	log_info(logger, "Conexion aceptada nuevo socket: %d", conexion_aceptada);

	return conexion_aceptada;
}

void * aceptar_proceso(void* funcion, Mensaje * mensaje) {
	pthread_t p_thread;
	pthread_create(&p_thread, NULL, funcion, mensaje);
}

/**
 * Recibe el mensaje en el socket, interpreta que es lo que require
 * Genera la respuesta y muere el hilo al enviarla
 * El proximo mensaje que se reciba de CPU va aparecer en el select del hilo principal
 * Y luego generara uno nuevo para que lo atiende otra vez 
 */
void * interpretar_mensajes_de_cpu(void* args){
	//For para recibir mensaje de cpu, interpretarlo y enviar la respuesta
	log_info(logger, "Soy el hilo %d, recibi una conexion en el socket: %d", process_get_thread_id(), ((Mensaje*)args)->socket);
	log_info(logger, "Interpreto mensaje");
}
