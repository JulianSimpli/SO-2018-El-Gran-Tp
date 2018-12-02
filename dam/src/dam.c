#include "dam.h"

void *mensajes_mdj();
void *mensajes_safa();
void *aceptar_cpu(int);
pthread_t levantar_hilo(void *);
void *interpretar_mensajes_de_cpu(void *);
void *interpretar_mensajes_de_mdj(void *);
void *interpretar_mensajes_de_safa(void *);
void *interpretar_mensajes_de_fm9(void *);
int crear_socket_fm9();
void aceptar_cpus();

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
	handshake_safa();
	log_info(logger, "Se concreto handshake SAFA");
	handshake_mdj();
	log_info(logger, "Se concreto handshake MDJ");
	//handshake_fm9();

	//inicializamos el semaforo en maximas_conexiones - 3 que son fijas

	pthread_t safa = levantar_hilo(interpretar_mensajes_de_safa);
	pthread_t mdj = levantar_hilo(interpretar_mensajes_de_mdj);
	//pthread_t fm9 = levantar_hilo(interpretar_mensajes_de_fm9);
	aceptar_cpus();
	pthread_join(safa, NULL);
	pthread_join(mdj, NULL);
	//pthread_join(fm9, NULL);
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
		int socket = accept(socket_ligado, NULL, NULL);

		if (socket < 0)
			_exit_with_error(socket_ligado, "Fallo el accept", NULL);

		log_info(logger, "Acepto cpu en socket: %d", socket);
		handshake_cpu(socket);
		log_info(logger, "Concrete handshake con cpu %d", socket);

		pthread_t p_thread;
		pthread_create(&p_thread, NULL, interpretar_mensajes_de_cpu, (void *)(intptr_t)socket);
	}
}

/**
 * Este handshake es el unico distinto porque recibe el nuevo socket
 */
void handshake_cpu(int socket)
{
	Paquete paquete;
	recibir_paquete(socket, &paquete);
	if (paquete.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket, "No se logro el handshake", NULL);
	EnviarHandshake(socket, ELDIEGO);
}

/**
 * Se encarga de crear el socket y mandar el primer mensaje
 * Devuelve el socket 
 */
void handshake_mdj()
{
	socket_mdj = crear_socket_mdj();

	Paquete config;
	config.header = cargar_header(INTSIZE, ESHANDSHAKE, ELDIEGO);
	config.Payload = malloc(INTSIZE);
	memcpy(config.Payload, transfer_size, INTSIZE);

	enviar_paquete(socket_mdj, &config);

	Paquete respuesta;
	recibir_paquete(socket_mdj, &respuesta);
	if (respuesta.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_mdj, "No se logro el handshake", config.Payload);
}

/**
 * Se encarga de crear el socket, mandar el primer mensaje
 * Recibir de fm9 el tamanio de linea que tiene el sistema
 * Devuelve el socket 
 */
void handshake_fm9()
{
	socket_fm9 = crear_socket_fm9();
	EnviarHandshake(socket_fm9, ELDIEGO);
	Paquete paquete;
	RecibirPaqueteCliente(socket_mdj, &paquete);
	if (paquete.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_fm9, "No se logro el primer paso de datos", NULL);
	//Guardo en la global tamanio de linea que tiene fm9
	memcpy(&tamanio_linea, paquete.Payload, sizeof(u_int32_t));
	log_info(logger, "Se concreto el handshake con FM9, me pasa el tamanio de linea %d", tamanio_linea);
	free(paquete.Payload);
}

/**
 * Se encarga de crear el socket, mandar el primer mensaje
 * Devuelve el socket 
 */
void handshake_safa()
{
	socket_safa = crear_socket_safa();
	EnviarHandshake(socket_safa, ELDIEGO);
	Paquete *paquete = malloc(sizeof(Paquete));
	log_debug(logger, "socket safa : %i", socket_safa);
	RecibirPaqueteCliente(socket_safa, paquete);

	if (paquete->header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_safa, "No se logro el primer paso de datos", NULL);
}

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
	Paquete paquete;
	RecibirPaqueteCliente(socket_mdj, &paquete);

	//interpreta
	Tipo accion = paquete.header.tipoMensaje;
	switch (accion)
	{
	case VALIDAR_ARCHIVO:
		RecibirPaqueteCliente(socket_mdj, &paquete);
		log_debug(logger, "El archivo tiene el tamanio");
		break;
	case ARCHIVO:
		break;
	default:
		log_warning(logger, "No se pudo interpretar el mensaje enviado por MDJ");
		break;
	}
	//enviar_mensaje
}

void *interpretar_mensajes_de_safa(void *args)
{
	Paquete paquete;
	RecibirPaqueteCliente(socket_safa, &paquete);
	switch (paquete.header.tipoMensaje)
	{
	case DTB_FAIL:
		break;
	default:
		log_error(logger, "No pude interpretar el mensaje");
		break;
	}
}

void *interpretar_mensajes_de_fm9(void *args)
{
	int socket = (intptr_t)args;
	//For para recibir mensaje de cpu, interpretarlo y enviar la respuesta
	log_info(logger, "Soy el hilo %d, recibi una conexion en el socket: %d", process_get_thread_id(), socket);
	log_info(logger, "Interpreto mensaje");
	Paquete paquete;
	RecibirPaqueteCliente(socket, &paquete);
	switch (paquete.header.tipoMensaje)
	{
	case CARGADO:
		log_info(logger, "FM9 alerto que ya cargo el archivo");
		break;
	}
}

void leer_config()
{
	config = config_create("/home/utnso/tp-2018-2c-Nene-Malloc/dam/src/DAM.config");
}

void inicializar_log(char *program)
{
	logger = log_create("DAM.log", program, true, LOG_LEVEL_DEBUG);
}

void inicializar(char **argv)
{
	inicializar_log(argv[0]);
	log_info(logger, "Logger cargado exitosamente");
	leer_config();
	log_info(logger, "Config cargada exitosamente");
	transfer_size = config_get_int_value(config, "TRANSFER_SIZE");
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

pthread_t levantar_hilo(void *funcion)
{
	pthread_t p_thread;
	pthread_create(&p_thread, NULL, funcion, NULL);
	return p_thread;
}

//TODO: Abstraer estas dos funciones
int pedir_validar(char *path)
{
	int len = strlen(path);
	Paquete validar;
	validar.header = cargar_header(len, VALIDAR_ARCHIVO, ELDIEGO);
	validar.Payload = malloc(len);
	memcpy(validar.Payload, path, len);
	EnviarPaquete(socket_mdj, &validar);
	Paquete respuesta;
	RecibirPaqueteCliente(socket_mdj, &respuesta);
	return respuesta.header.tipoMensaje != PATH_INEXISTENTE;
}

int cargar_a_memoria(u_int32_t pid, char *path, char *file, Paquete *respuesta)
{
	int desplazamiento = 0;
	int tam_pid = sizeof(u_int32_t);
	int tam_serial_path = 0;
	int tam_serial_file = 0;

	void *serial_path = string_serializar(path, &tam_serial_path);
	void *serial_file = string_serializar(file, &tam_serial_file);
	
	// payload = pid, len path, path, len archivo, archivo	
	Paquete cargar;
	cargar.Payload = malloc(tam_pid + tam_serial_path + tam_serial_file);
	memcpy(cargar.Payload, &pid, tam_pid);
	desplazamiento += tam_pid;
	memcpy(cargar.Payload + desplazamiento, serial_path, tam_serial_path);
	desplazamiento += tam_serial_path;
	memcpy(cargar.Payload + desplazamiento, serial_file, tam_serial_file);
	desplazamiento += tam_serial_file;

	cargar.header = cargar_header(desplazamiento, ABRIR, ELDIEGO);
	EnviarPaquete(socket_fm9, &cargar);
	
	free(serial_path);
	free(serial_file);
	free(cargar.Payload);

	RecibirPaqueteCliente(socket_fm9, respuesta);
	return respuesta.header.tipoMensaje != ESPACIO_INSUFICIENTE_ABRIR;
}

void enviar_error(Tipo tipo, int pid)
{
	Paquete error;
	error.Payload = malloc(INTSIZE);
	memcpy(error.Payload, pid, INTSIZE);
	error.header = cargar_header(INTSIZE, tipo, ELDIEGO);
	enviar_paquete(socket_safa, &error);
	log_error(logger, "El proceso %d fallo por %d", pid, tipo);
	free(error.Payload);
}

void enviar_success(Tipo tipo, int pid)
{
	Paquete respuesta;
	respuesta.header = cargar_header(INTSIZE, tipo, ELDIEGO);
	respuesta.Payload = malloc(INTSIZE);
	memcpy(respuesta.Payload, &pid, INTSIZE);
	enviar_paquete(socket_safa, &respuesta);
	free(respuesta.Payload);
}

/**
 * Recibe el mensaje en el socket, interpreta que es lo que require
 * Genera la respuesta y muere el hilo al enviarla
 * El proximo mensaje que se reciba de CPU va aparecer en el select del hilo principal
 * Y luego generara uno nuevo para que lo atiende otra vez 
 */
void *interpretar_mensajes_de_cpu(void *arg)
{
	int socket = (intptr_t)arg;
	//For para recibir mensaje de cpu, interpretarlo y enviar la respuesta
	log_debug(logger, "Soy el hilo %d, recibi una conexion en el socket: %d", process_get_thread_id(), socket);
	log_info(logger, "Interpreto nuevo mensaje");
	Paquete paquete;
	recibir_paquete(socket, &paquete);

	switch (paquete.header.tipoMensaje)
	{
	case ESDTBDUMMY:
		DTB *dummy = DTB_deserializar(paquete.Payload);
		ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dummy);
		log_debug(logger, "Pido que me devuelva el escriptorio %s", escriptorio->path);

		int validar = pedir_validar(escriptorio->path);

		if (!validar) {
			enviar_error(DUMMY_FAIL, dummy->gdtPID);
			break;
		}

		Paquete pedido_escriptorio;
		pedido_escriptorio.header = cargar_header(0, OBTENER_DATOS, ELDIEGO);
		char *primitiva = escriptorio->path;
		EnviarPaquete(socket_mdj, &pedido_escriptorio);

		log_debug(logger, "Le envie el pedido a mdj");
		RecibirPaqueteCliente(socket_mdj, &paquete);
		log_debug(logger, "Le envie el pedido a mdj");
		log_debug(logger, "Recibi %s", (char *)paquete.Payload);

		int cargado = cargar_a_memoria(escriptorio->path);

		if (!cargado) {
			enviar_error(DUMMY_FAIL, dummy->gdtPID);
			break;
		}

		enviar_success(DUMMY_SUCCESS, dummy->gdtPID);
		break;
	case ABRIR:
		int pid = 0;
		memcpy(&pid, paquete.Payload, INTSIZE);
		char *path = string_deserializar(paquete.Payload, INTSIZE);
		log_debug(logger, "Recibi el payload %s", path);

		int validar = pedir_validar(path);

		if (!validar) {
			enviar_error(PATH_INEXISTENTE, pid);
			break;
		}

		int cargado = cargar_a_memoria(escriptorio->path);

		if (!cargado) {
			enviar_error(ESPACIO_INSUFICIENTE_ABRIR, pid);
			break;
		}

		//TODO: Tengo que traerme de cpu la flag para saber si es dummy
		enviar_success(DUMMY_SUCCESS, pid);
		//enviar_success(DTB_SUCCESS, pid);

		break;
	case CREAR_ARCHIVO:

		int desplazamiento = 0, pid = 0;
		memcpy(&pid, paquete.Payload + desplazamiento, INTSIZE);
		desplazamiento += INTSIZE;

		char *path = string_deserializar(paquete.Payload, &desplazamiento);
		int lineas = 0;
		memcpy(&lineas, paquete.Payload + desplazamiento, INTSIZE);

		//En este caso si el archivo existe tira error
		int validar = pedir_validar(escriptorio->path);

		if (validar)
		{
			enviar_error(socket_safa, ARCHIVO_YA_EXISTENTE, pid);
			break;
		}

		//Reenvio el pedido de crear archivo a mdj
		//Pero le saco el pid que no lo necesita
		paquete.header.tamPayload -= INTSIZE;
		paquete.Payload = paquete.Payload + INTSIZE;
		enviar_paquete(socket_mdj, &paquete);

		Paquete respuesta_mdj;
		recibir_paquete(socket_mdj, &respuesta_mdj);

		if (respuesta_mdj.header.tipoMensaje == ESPACIO_INSUFICIENTE_CREAR)
		{
			enviar_error(socket_safa, ESPACIO_INSUFICIENTE_CREAR, dummy->gdtPID);
			break;
		}

		enviar_success(DTB_SUCCESS, pid);
		break;

	case BORRAR_ARCHIVO:

		int desplazamiento = 0, pid = 0;
		memcpy(&pid, paquete.Payload + desplazamiento, INTSIZE);
		desplazamiento += INTSIZE;

		char *path = string_deserializar(paquete.Payload, &desplazamiento);

		log_debug(logger, "Envio a MDJ");
		//Reenvio el pedido de crear archivo a mdj
		//Pero le saco el pid que no lo necesita
		paquete.Payload = paquete.Payload + INTSIZE;
		enviar_paquete(socket_mdj, &paquete);

		Paquete respuesta_mdj;
		recibir_paquete(socket_mdj, &respuesta_mdj);

		if (respuesta_mdj.header.tipoMensaje == ARCHIVO_NO_EXISTE) {
			enviar_error(socket_safa, ARCHIVO_NO_EXISTE, pid);
			break;
		}

		enviar_success(DTB_SUCCESS, pid);
		break;

	case FLUSH:

		int desplazamiento = 0, pid = 0;
		memcpy(&pid, paquete.Payload + desplazamiento, INTSIZE);
		desplazamiento += INTSIZE;

		char *path = string_deserializar(paquete.Payload, &desplazamiento);

		log_debug(logger, "Envio a FM9");
		//enviar_paquete(socket_fm9, &paquete);

		Paquete respuesta_fm9;
		recibir_paquete(socket_fm9, &respuesta_fm9);

		if (respuesta_fm9.header.tipoMensaje == FALLO_DE_SEGMENTO) {
			enviar_error(FALLO_DE_SEGMENTO, pid);
			break;
		}

		log_debug(logger, "Recibi el archivo entero y pesa %d", respuesta_fm9.header.tamPayload);
		Paquete pedido_escritura;
		pedido_escritura.header = cargar_header(size, GUARDAR_DATOS, ELDIEGO);
		pedido_escritura.Payload = malloc(size);
		//TODO: copiar el len como tercer parametro para mdj
		//TODO: copiar todo el payload de fm9 al nuevo paquete de mdj
		int offset = 0;
		paquete.Payload = paquete.Payload + INTSIZE;
		enviar_paquete(socket_mdj, &paquete);
		Paquete respuesta_fm9;
		recibir_paquete(socket_fm9, &respuesta_fm9);

		if (respuesta_mdj.header.tipoMensaje == ESPACIO_INSUFICIENTE_FLUSH) {
			enviar_error(ARCHIVO_NO_EXISTE_FLUSH, pid);
			break;
		}

		if (respuesta_mdj.header.tipoMensaje == ARCHIVO_NO_EXISTE_FLUSH) {
			enviar_error(ARCHIVO_NO_EXISTE_FLUSH, pid);
			break;
		}

		enviar_success(DTB_SUCCESS, pid);
		break;

	default:
		log_error(logger, "No entiendo el mensaje");
		break;
	}
	free(paquete.Payload);
}
