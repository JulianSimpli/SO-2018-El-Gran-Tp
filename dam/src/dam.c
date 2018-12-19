#include "dam.h"

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
	handshake_fm9();
	log_info(logger, "Se concreto handshake FM9");

	//inicializamos el semaforo en maximas_conexiones - 3 que son fijas

	pthread_t safa = levantar_hilo(interpretar_mensajes_de_safa);
	aceptar_cpus();
	pthread_join(safa, NULL);
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

//Creacion sockets

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

int crear_socket_mdj()
{
	char *puerto_mdj = config_get_string_value(config, "PUERTO_MDJ");

	char *ip_mdj = config_get_string_value(config, "IP_MDJ");

	return connect_to_server(ip_mdj, puerto_mdj);
}

//Handshakes

void enviar_handshake(int socket)
{
	Paquete handshake;
	handshake.header = cargar_header(INTSIZE, ESHANDSHAKE, ELDIEGO);
	handshake.Payload = malloc(INTSIZE);
	memcpy(handshake.Payload, &transfer_size, INTSIZE);
	log_debug(logger, "Enviar handshake");
	enviar_paquete(socket, &handshake);
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

	enviar_handshake(socket);
	// free(paquete.Payload);
}

/**
 * Se encarga de crear el socket y mandar el primer mensaje
 * Devuelve el socket 
 */
void handshake_mdj()
{
	socket_mdj = crear_socket_mdj();

	enviar_handshake(socket_mdj);

	Paquete respuesta;
	recibir_paquete(socket_mdj, &respuesta);
	if (respuesta.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_mdj, "No se logro el handshake", respuesta.Payload);
}
/**
 * Se encarga de crear el socket, mandar el primer mensaje
 * Recibir de fm9 el tamanio de linea que tiene el sistema
 * Devuelve el socket 
 */
void handshake_fm9()
{
	socket_fm9 = crear_socket_fm9();

	enviar_handshake(socket_fm9);

	Paquete paquete;
	recibir_paquete(socket_fm9, &paquete);

	if (paquete.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_fm9, "No se logro el primer paso de datos", NULL);

	//Guardo en la global tamanio de linea que tiene fm9
	memcpy(&tamanio_linea, paquete.Payload, INTSIZE);
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

	enviar_handshake(socket_safa);

	sleep(2);

	Paquete paquete;
	recibir_paquete(socket_safa, &paquete);

	if (paquete.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_safa, "No se logro el primer paso de datos", NULL);
}

void *interpretar_mensajes_de_safa(void *args)
{
	Paquete paquete;
	Paquete respuesta;
	while (1)
	{
		recibir_paquete(socket_safa, &paquete);
		int tam_posicion = 0;
		Posicion *posicion = deserializar_posicion(paquete.Payload, &tam_posicion);
		log_posicion(logger, posicion, "Recibo posicion a finalizar");

		if (paquete.header.tipoMensaje != FINALIZAR)
			_exit_with_error(socket_safa, "No pude interpretar el mensaje", paquete.Payload);

		paquete.header.emisor = ELDIEGO;

		log_debug(logger, "Envio finalizar a fm9");
		enviar_paquete(socket_fm9, &paquete);

		recibir_paquete(socket_fm9, &respuesta);

		log_debug(logger, "tipo mensaje %d", respuesta.header.tipoMensaje);
		if (respuesta.header.tipoMensaje != SUCCESS)
			_exit_with_error(socket_fm9, "No pudo liberar la memoria de un proceso", respuesta.Payload);

		log_debug(logger, "Se pudo liberar la memoria");
		respuesta.header.emisor = ELDIEGO;
		respuesta.header.tipoMensaje = DTB_FINALIZAR;
		enviar_paquete(socket_safa, &respuesta);
		log_debug(logger, "Envie finalizar a safa");

		free(paquete.Payload);
		free(respuesta.Payload);
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

void inicializar_semaforos()
{
	sem_init(&sem_mdj, 0, 1);
}

void inicializar(char **argv)
{
	inicializar_log(argv[0]);
	log_info(logger, "Logger cargado exitosamente");
	leer_config();
	log_info(logger, "Config cargada exitosamente");
	transfer_size = config_get_int_value(config, "TRANSFER_SIZE");
	log_debug(logger, "Transfer size: %i", transfer_size);
	inicializar_semaforos();
}

int escuchar_conexiones(int server_socket)
{

	log_info(logger, "Empiezo a escuchar el socket %d", server_socket);

	int conexion_aceptada = accept(server_socket, NULL, NULL);

	if (conexion_aceptada < 0)
		_exit_with_error(server_socket, "Fallo el accept", NULL);

	log_info(logger, "Conexion aceptada nuevo socket: %d", conexion_aceptada);

	return conexion_aceptada;
}

pthread_t levantar_hilo(void *funcion)
{
	pthread_t p_thread;
	pthread_create(&p_thread, NULL, funcion, NULL);
	return p_thread;
}

int pedir_validar(char *path)
{
	int desplazamiento = 0;
	void *serializado = string_serializar(path, &desplazamiento);
	log_debug(logger, "Path %s", path);
	int b = 0;
	char *prueba = string_deserializar(serializado, &b);

	log_debug(logger, "Prueba %s", prueba);

	Paquete validar;
	validar.header = cargar_header(desplazamiento, VALIDAR_ARCHIVO, ELDIEGO);
	validar.Payload = malloc(desplazamiento);
	memcpy(validar.Payload, serializado, desplazamiento);

	enviar_paquete(socket_mdj, &validar);
	free(serializado);

	Paquete respuesta;
	sem_wait(&sem_mdj);
	recibir_paquete(socket_mdj, &respuesta);
	log_debug(logger, "Recibi el tipo de mensaje %d", respuesta.header.tipoMensaje);
	sem_post(&sem_mdj);

	//Da lo mismo preguntar por si viene vacio el payload o si es el error PATH_INEXISTENTE
	if (respuesta.header.tamPayload > 0)
	{
		int size = 0;
		memcpy(&size, respuesta.Payload, INTSIZE);
		return size;
	}

	return 0;
}

int cargar_a_memoria(u_int32_t pid, char *path, char *file, Paquete *respuesta)
{
	int desplazamiento = 0;
	int tam_pid = sizeof(u_int32_t);
	int tam_serial_path = 0;
	int tam_serial_file = 0;

	void *serial_path = string_serializar(path, &tam_serial_path);
	void *serial_file = string_serializar(file, &tam_serial_file);

	log_debug(logger, "Cargo el paquete con:\npid %d\ntam_serial_path: %d\npath: %s\ntam_serial_file: %d\nfile:\n%s",
		  pid, tam_serial_path, path, tam_serial_file, file);
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
	log_debug(logger, "Total a enviar: %d", desplazamiento);
	enviar_paquete(socket_fm9, &cargar);

	log_debug(logger, "Pedido de carga a memoria a FM9 enviado");
	free(serial_path);
	free(serial_file);
	free(cargar.Payload);

	recibir_paquete(socket_fm9, respuesta); // Ahora se queda en este recv. Hay que armar el paquete fm9->dam
	//EnviarDatosTipo(te envia un paquete con el payload en 0 y tiene que enviar la respuesta con el archivo abierto)
	log_debug(logger, "FM9 respondio el pedido de carga a memoria");
	return respuesta->header.tipoMensaje != ESPACIO_INSUFICIENTE_ABRIR;
}

void enviar_error(Tipo tipo, int pid)
{
	Paquete error;
	error.Payload = malloc(INTSIZE);
	memcpy(error.Payload, &pid, INTSIZE);
	error.header = cargar_header(INTSIZE, tipo, ELDIEGO);
	enviar_paquete(socket_safa, &error);
	log_error(logger, "El proceso %d fallo por %d", pid, tipo);
	//free(error.Payload);
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

	log_debug(logger, "Soy el hilo %d, recibi una conexion en el socket: %d", process_get_thread_id(), socket);
	log_info(logger, "Interpreto nuevo mensaje");

	Paquete paquete;

	while (1)
	{
		recibir_paquete(socket, &paquete);

		switch (paquete.header.tipoMensaje)
		{
		case ESDTBDUMMY:
			log_info(logger, "ESDTBDUMMY");
			es_dtb_dummy(&paquete);
			break;
		case ABRIR:
			log_info(logger, "ABRIR");
			abrir(&paquete);
			break;
		case CREAR_ARCHIVO:
			log_info(logger, "CREAR_ARCHIVO");
			crear_archivo(&paquete);
			break;
		case BORRAR_ARCHIVO:
			log_debug(logger, "BORRAR_ARCHIVO");
			borrar_archivo(&paquete);
			break;
		case FLUSH:
			log_debug(logger, "FLUSH");
			flush(&paquete);
			break;
		default:
			log_error(logger, "No entiendo el mensaje");
			break;
		}

		free(paquete.Payload);
	}
}

void enviar_obtener_datos(char *path, int size)
{
	Paquete pedido_escriptorio;
	int desplazamiento = 0;
	void *serializado = string_serializar(path, &desplazamiento);

	pedido_escriptorio.Payload = malloc(desplazamiento + 2 * INTSIZE);

	memcpy(pedido_escriptorio.Payload, serializado, desplazamiento);
	int offset = 0;
	memcpy(pedido_escriptorio.Payload + desplazamiento, &offset, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(pedido_escriptorio.Payload + desplazamiento, &size, INTSIZE);
	desplazamiento += INTSIZE;

	pedido_escriptorio.header = cargar_header(desplazamiento, OBTENER_DATOS, ELDIEGO);

	enviar_paquete(socket_mdj, &pedido_escriptorio);
	free(serializado);
	free(pedido_escriptorio.Payload);
}

void enviar_guardar_datos(Paquete *paquete)
{
	int desplazamiento = 0, tam_path = 0, tam_file = 0;
	char *path = string_deserializar(paquete->Payload, &desplazamiento);
	char *file = string_deserializar(paquete->Payload, &desplazamiento);
	void *serial_path = string_serializar(path, &tam_path);
	void *serial_file = string_serializar(file, &tam_file);

	Paquete guardar_mdj;
	u_int32_t offset = 0;
	u_int32_t len = strlen(file);
	desplazamiento = 0;
	guardar_mdj.header = cargar_header(INTSIZE * 2 + tam_path + tam_file, GUARDAR_DATOS, ELDIEGO);
	guardar_mdj.Payload = malloc(guardar_mdj.header.tamPayload);
	memcpy(guardar_mdj.Payload + desplazamiento, serial_path, tam_path);
	desplazamiento += tam_path;
	memcpy(guardar_mdj.Payload + desplazamiento, &offset, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(guardar_mdj.Payload + desplazamiento, &len, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(guardar_mdj.Payload + desplazamiento, serial_file, tam_file);
	desplazamiento += tam_file;

	enviar_paquete(socket_mdj, &guardar_mdj);
	log_header(logger, &guardar_mdj, "Envie guardar datos a MDJ");
	log_debug(logger, "Payload:\nPath: %s\nbytes_offset: %d\nbuffer_size: %d\nFile:\n%s",
		  path, offset, len, file);

	free(path);
	free(file);
	free(serial_path);
	free(serial_file);
	free(guardar_mdj.Payload);
}

void es_dtb_dummy(Paquete *paquete)
{
	DTB *dummy = DTB_deserializar(paquete->Payload);
	ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dummy);
	log_debug(logger, "Pido que me devuelva el escriptorio %s", escriptorio->path);

	int validar = pedir_validar(escriptorio->path);

	log_debug(logger, "El archivo pesa %d bytes", validar);

	if (validar == 0)
	{
		enviar_error(DUMMY_FAIL_NO_EXISTE, dummy->gdtPID);
		return;
	}

	enviar_obtener_datos(escriptorio->path, validar);
	sem_wait(&sem_mdj);

	Paquete file_mdj;
	recibir_paquete(socket_mdj, &file_mdj);
	sem_post(&sem_mdj);
	int d = 0;
	char *script = string_deserializar(file_mdj.Payload, &d);
	log_debug(logger, "Escriptorio \n%s", script);

	log_debug(logger, "Le envio el pedido a fm9");

	Paquete respuesta;
	int cargado = cargar_a_memoria(dummy->gdtPID, escriptorio->path, script, &respuesta);

	if (!cargado)
	{
		enviar_error(DUMMY_FAIL_CARGA, dummy->gdtPID);
		return;
	}

	respuesta.header = cargar_header(respuesta.header.tamPayload, DUMMY_SUCCESS, ELDIEGO);
	enviar_paquete(socket_safa, &respuesta);
	free(respuesta.Payload);
}

void abrir(Paquete *paquete)
{
	int desplazamiento = 0, pid = 0;
	memcpy(&pid, paquete->Payload, INTSIZE);
	desplazamiento += INTSIZE;
	char *path = string_deserializar(paquete->Payload, &desplazamiento);
	log_debug(logger, "Recibi el payload %s", path);

	int validar = pedir_validar(path);

	if (!validar)
	{
		enviar_error(PATH_INEXISTENTE, pid);
		return;
	}

	enviar_obtener_datos(path, validar);
	log_debug(logger, "Le envie el pedido a mdj");
	sem_wait(&sem_mdj);

	Paquete file_mdj;
	recibir_paquete(socket_mdj, &file_mdj);
	log_debug(logger, "Recibi el pedido a mdj");
	sem_post(&sem_mdj);
	int d = 0;
	char *script = string_deserializar(file_mdj.Payload, &d);
	log_debug(logger, "Archivo a abrir:\n%s", script);

	Paquete respuesta;
	int cargado = cargar_a_memoria(pid, path, script, &respuesta);

	if (!cargado)
	{
		enviar_error(ESPACIO_INSUFICIENTE_ABRIR, pid);
		return;
	}

	respuesta.header = cargar_header(respuesta.header.tamPayload, ARCHIVO_ABIERTO, ELDIEGO);
	enviar_paquete(socket_safa, &respuesta);
	free(respuesta.Payload);
	free(file_mdj.Payload);
}

void flush(Paquete *paquete)
{
	int desplazamiento = 0;
	int pid = 0;
	memcpy(&pid, paquete->Payload, INTSIZE);
	desplazamiento += 4 * INTSIZE;
	char *path = string_deserializar(paquete->Payload, &desplazamiento);

	log_debug(logger, "Envio a FM9");
	paquete->header = cargar_header(paquete->header.tamPayload, FLUSH, ELDIEGO);

	enviar_paquete(socket_fm9, paquete);

	Paquete respuesta_fm9;
	recibir_paquete(socket_fm9, &respuesta_fm9);
	log_header(logger, &respuesta_fm9, "Recibi respuesta a flush GDT %d de %s", pid, path);

	log_debug(logger, "Tipo msje FM9:%d", respuesta_fm9.header.tipoMensaje);

	if (respuesta_fm9.header.tipoMensaje == FALLO_DE_SEGMENTO_FLUSH)
	{
		enviar_error(FALLO_DE_SEGMENTO_FLUSH, pid);
		return;
	}

	enviar_guardar_datos(&respuesta_fm9);

	Paquete respuesta_mdj;
	sem_wait(&sem_mdj);
	recibir_paquete(socket_mdj, &respuesta_mdj);
	sem_post(&sem_mdj);

	if (respuesta_mdj.header.tipoMensaje == ESPACIO_INSUFICIENTE_FLUSH)
	{
		enviar_error(ESPACIO_INSUFICIENTE_FLUSH, pid);
		return;
	}

	if (respuesta_mdj.header.tipoMensaje == ARCHIVO_NO_EXISTE_FLUSH)
	{
		enviar_error(ARCHIVO_NO_EXISTE_FLUSH, pid);
		return;
	}

	enviar_success(DTB_SUCCESS, pid);
	free(path);
}

void crear_archivo(Paquete *paquete)
{
	int desplazamiento = 0, pid = 0;
	int lineas = 0;
	memcpy(&pid, paquete->Payload + desplazamiento, INTSIZE);
	desplazamiento += INTSIZE;

	char *path = string_deserializar(paquete->Payload, &desplazamiento);

	//En este caso si el archivo existe tira error
	int validar = pedir_validar(path);

	if (validar)
	{
		enviar_error(ARCHIVO_YA_EXISTENTE, pid);
		return;
	}

	//Para no terminar deserializar y volver a serializar
	//Reenvio el pedido de crear archivo a mdj
	paquete->header.tamPayload -= INTSIZE; //No necesito enviarle el pid
	Paquete pedido;
	pedido.header = cargar_header(paquete->header.tamPayload, CREAR_ARCHIVO, ELDIEGO);
	pedido.Payload = malloc(paquete->header.tamPayload);
	int len = paquete->header.tamPayload - INTSIZE; //Lo que hay en el payload menos la cant_lineas
	//Reutilizo el paquete payload pero con el offset del pid
	memcpy(pedido.Payload, paquete->Payload + INTSIZE, len);
	memcpy(pedido.Payload + len, paquete->Payload + INTSIZE + len, INTSIZE);

	enviar_paquete(socket_mdj, &pedido);

	Paquete respuesta_mdj;
	sem_wait(&sem_mdj);
	recibir_paquete(socket_mdj, &respuesta_mdj);
	sem_post(&sem_mdj);

	if (respuesta_mdj.header.tipoMensaje == ESPACIO_INSUFICIENTE_CREAR)
	{
		enviar_error(ESPACIO_INSUFICIENTE_CREAR, pid);
		return;
	}

	enviar_success(DTB_SUCCESS, pid);
	free(path);
	free(pedido.Payload);
}

void borrar_archivo(Paquete *paquete)
{
	int desplazamiento = 0, pid = 0;
	memcpy(&pid, paquete->Payload + desplazamiento, INTSIZE);
	desplazamiento += INTSIZE;

	char *path = string_deserializar(paquete->Payload, &desplazamiento);

	log_debug(logger, "Envio a MDJ");
	//Para no terminar deserializar y volver a serializar
	//Reenvio el pedido de borrar archivo a mdj
	//no necesito mandarle el PID
	Paquete pedido;
	int len = paquete->header.tamPayload - INTSIZE; //Lo que hay en el payload menos el pid
	pedido.header = cargar_header(paquete->header.tamPayload, BORRAR_ARCHIVO, ELDIEGO);
	pedido.Payload = malloc(paquete->header.tamPayload);
	//Reutilizo el paquete payload pero con el offset del pid
	memcpy(pedido.Payload, paquete->Payload + INTSIZE, len);

	enviar_paquete(socket_mdj, &pedido);

	Paquete respuesta_mdj;
	sem_wait(&sem_mdj);
	recibir_paquete(socket_mdj, &respuesta_mdj);
	sem_post(&sem_mdj);

	if (respuesta_mdj.header.tipoMensaje == ARCHIVO_NO_EXISTE)
	{
		enviar_error(ARCHIVO_NO_EXISTE, pid);
		return;
	}

	enviar_success(DTB_SUCCESS, pid);
	free(pedido.Payload);
}
