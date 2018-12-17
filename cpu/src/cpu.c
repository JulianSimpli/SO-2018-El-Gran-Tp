#include "cpu.h"
#include <commons/string.h>

int quantum;
char *algoritmo;
int socket_dam;
int socket_safa;
int socket_fm9;
int retardo;
sem_t sem_recibir_paquete;
sem_t sem_senial;
int transfer_size = 0;
int senial_safa;

sem_t *cambios;
bool finalizar;
void *hilo_safa();
int interpretar_safa(Paquete *paquete);
char *pedir_primitiva(DTB *dtb);
void cargar_config_safa(Paquete *paquete);
int ejecutar(char *, DTB *);

int main(int argc, char **argv)
{
	inicializar(argv);

	handshake_safa();
	log_debug(logger, "Concrete handshake con safa");
	handshake_dam();
	log_debug(logger, "Concrete handshake con dam");
	handshake_fm9();
	sem_init(&sem_recibir_paquete, 0, 1);
	sem_init(&sem_senial, 0, 1);
	pthread_t p_thread_one;
	pthread_create(&p_thread_one, NULL, hilo_safa, NULL);
	pthread_t p_thread_two;
	pthread_create(&p_thread_two, NULL, hilo_safa, NULL);

	pthread_join(p_thread_one, NULL);
	pthread_join(p_thread_two, NULL);
	return 0;
}

void *hilo_safa()
{
	while (1)
	{
		Paquete *paquete = malloc(sizeof(Paquete));
		sem_wait(&sem_recibir_paquete);
		log_info(logger, "Soy el hilo %d, espero paquete de safa", process_get_thread_id());
		int r = RecibirPaqueteCliente(socket_safa, paquete);
		if (r <= 0)
			_exit_with_error(socket_safa, "Se desconecto safa", paquete);
		sem_post(&sem_recibir_paquete);
		interpretar_safa(paquete);
	}
}

int ejecutar(char *linea, DTB *dtb)
{
	int i = 0, existe = 0, flag = 1;
	//Calcula la cantidad de primitivas que hay en el array
	size_t cantidad_primitivas = sizeof(primitivas) / sizeof(primitivas[0]);

	//Para poder identificar el comando y que el resto son parametros necesarios para que ejecute
	//Por ej: linea = abrir /home/utnso/
	char **parameters = string_n_split(linea, 4, " ");

	for (i; i < cantidad_primitivas; i++)
	{
		if (!strcmp(primitivas[i].name, parameters[0]))
		{
			dtb->PC++;
			log_debug(logger, "Interprete %s", primitivas[i].name);
			existe = 1;
			log_info(logger, primitivas[i].doc);
			//llama a la funcion que tiene guardado esa primitiva en la estructura
			sleep(retardo);
			flag = primitivas[i].func(parameters, dtb);
			break;
		}
	}

	if (!existe)
		_exit_with_error(-1, "No existe la primitiva", NULL);

	return flag;
}

int ejecutar_algoritmo(Paquete *paquete)
{
	log_debug(logger, "Voy a deserializar el dtb");
	DTB *dtb = DTB_deserializar(paquete->Payload);
	log_debug(logger, "Deserialice el dtb");
	log_debug(logger, "pid %d", dtb->gdtPID);
	Paquete *nuevo_paquete = malloc(sizeof(Paquete));
	int tam_pid_y_pc = 0;
	ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
	int cantidad_lineas = escriptorio->cantLineas;
	int flag = 1;
	if (!strcmp(algoritmo, "FIFO"))
	{
		while (1)
		{
			char *primitiva = pedir_primitiva(dtb);
			log_debug(logger, "Primitiva %s", primitiva);
			if (!strcmp(primitiva, "Fallo"))
			{
				log_debug(logger, "Fallo el pedido de primitiva");
				break;
			}
			flag = ejecutar(primitiva, dtb);
			log_debug(logger, "Voy por %d y tengo %d cantidad de lineas", dtb->PC, cantidad_lineas);
			if (flag || finalizar)
			{
				finalizar = false;
				break;
			}
			if (cantidad_lineas == dtb->PC)
				return enviar_pid_y_pc(dtb, DTB_FINALIZAR); 
		}
	}
	else
	{
		int quantum_local = quantum;
		if (paquete->header.tipoMensaje == ESDTBQUANTUM)
			memcpy(&quantum_local, paquete->Payload + paquete->header.tamPayload - INTSIZE, INTSIZE);
		int i;

		for (i = 0; i < quantum_local; i++)
		{
			log_debug(logger, "Quantum %d/%d", i, quantum_local);
			char *primitiva = pedir_primitiva(dtb);
			if (!strcmp(primitiva, "Fallo"))
			{
				log_debug(logger, "Fallo el pedido de primitiva");
				break;
			}
			log_debug(logger, "Primitiva %s", primitiva);
			flag = ejecutar(primitiva, dtb); //avanzar el PC dentro del paquete
			log_debug(logger, "Flag %d", flag);
			log_debug(logger, "Voy por %d y tengo %d lineas", dtb->PC, cantidad_lineas);
			if (flag || finalizar)
			{
				finalizar = false;
				break;
			}

			if (cantidad_lineas == dtb->PC)
				return enviar_pid_y_pc(dtb, DTB_FINALIZAR);
		}
		//armar el paquete para mandar a safa, que dependiendo de si es RR o VRR, envía quantum restante
		//si es RR, manda DTB_EJECUTO
		//si es VRR, manda QUANTUM_FALTANTE
		if (!strcmp(algoritmo, "VRR"))
			return enviar_pid_y_pc(dtb, QUANTUM_FALTANTE);
	}

	if (!flag)
		return enviar_pid_y_pc(dtb, DTB_EJECUTO);

	dtb_liberar(dtb);
	return 1;
}

char *pedir_primitiva(DTB *dtb)
{
	Paquete *paquete = malloc(sizeof(Paquete));
	ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
	int len_int = INTSIZE * 2;
	int desplazamiento = 0;
	int size_archivo = 0;
	void *serializado = DTB_serializar_archivo(escriptorio, &size_archivo);

	paquete->Payload = malloc(len_int + size_archivo);

	memcpy(paquete->Payload, &dtb->gdtPID, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(paquete->Payload + desplazamiento, &dtb->PC, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(paquete->Payload + desplazamiento, serializado, size_archivo);
	desplazamiento += size_archivo;

	paquete->header = cargar_header(desplazamiento, NUEVA_PRIMITIVA, CPU);

	log_debug(logger, "Pido a fm9 siguiente primitiva");

	int ret = EnviarPaquete(socket_fm9, paquete);
	if (ret != -1)
	{
		log_debug(logger, "Le pedi primitiva");
		Paquete *primitiva_recibida = malloc(sizeof(Paquete));
		RecibirPaqueteCliente(socket_fm9, primitiva_recibida);
		log_debug(logger, "%d", primitiva_recibida->header.tipoMensaje);
		log_debug(logger, "%d", primitiva_recibida->header.tamPayload);
		if (primitiva_recibida->header.tipoMensaje == ERROR)
			return "Fallo";
		char *primitiva = malloc(primitiva_recibida->header.tamPayload);
		memcpy(primitiva, primitiva_recibida->Payload, primitiva_recibida->header.tamPayload);
		free(primitiva_recibida->Payload);
		free(primitiva_recibida);
		return primitiva;
	}
}

int interpretar_safa(Paquete *paquete)
{
	log_info(logger, "%d: Interpreto el tipo de mensaje", process_get_thread_id());
	switch (paquete->header.tipoMensaje)
	{
	case ESDTBDUMMY:
		log_debug(logger, "ESDTBDUMMY");
		EnviarPaquete(socket_dam, paquete); //envia el dtb al diego para que este haga el pedido correspondiente a MDJ
		bloquea_dummy(paquete);
		break;
	case ESDTB:
		log_debug(logger, "ESDTB");
		ejecutar_algoritmo(paquete);
		break;
	case ESDTBQUANTUM:
		log_debug(logger, "ESDTBQUANTUM");
		ejecutar_algoritmo(paquete);
		break;
	case ROJADIRECTA:
		senial_safa = ROJADIRECTA;
		sem_post(&sem_senial);
		break;
	case SIGASIGA:
		senial_safa = SIGASIGA;
		sem_post(&sem_senial);
		break;
	case FINALIZAR:
		finalizar = true;
		break;
	case CAMBIO_CONFIG:
		cargar_config_safa(paquete);
		break;
	default:
		break;
	}
}

void bloquea_dummy(Paquete *paquete)
{
	DTB *dtb = DTB_deserializar(paquete->Payload);
	Paquete dummy;
	dummy.Payload = malloc(INTSIZE);
	dummy.header = cargar_header(INTSIZE, DUMMY_BLOQUEA, CPU);
	memcpy(dummy.Payload, &dtb->gdtPID, INTSIZE);
	EnviarPaquete(socket_safa, &dummy);
}

void handshake_safa()
{
	socket_safa = crear_socket_safa();
	EnviarHandshake(socket_safa, CPU);
	Paquete *paquete = malloc(sizeof(Paquete));
	RecibirPaqueteCliente(socket_safa, paquete);
	if (paquete->header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_safa, "No se logro el handshake", NULL);
	//TODO: Recibe quantum por primera vez de SAFA
	cargar_config_safa(paquete);

	log_info(logger, "Se concreto el handshake con SAFA, empiezo a recibir mensajes");
	log_debug(logger, "Quantum: %d", quantum);
	log_debug(logger, "Algoritmo de planificacion: %s", algoritmo);
}

void handshake_dam()
{
	socket_dam = crear_socket_dam();
	EnviarHandshake(socket_dam, CPU);
	Paquete paquete;
	RecibirDatos(&paquete.header, socket_dam, TAMANIOHEADER);
	paquete.Payload = malloc(paquete.header.tamPayload);
	RecibirDatos(paquete.Payload, socket_dam, paquete.header.tamPayload);
	memcpy(&transfer_size, paquete.Payload, paquete.header.tamPayload);
	if (paquete.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_dam, "No se logro el handshake", NULL);

	log_info(logger, "Se concreto el handshake con DAM, empiezo a recibir mensajes");
}

void handshake_fm9()
{
	socket_fm9 = crear_socket_fm9();
	EnviarHandshake(socket_fm9, CPU);
	Paquete paquete;
	RecibirDatos(&paquete.header, socket_fm9, TAMANIOHEADER);
	if (paquete.header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_dam, "No se logro el handshake", NULL);

	log_info(logger, "Se concreto el handshake con FM9");
}

void cargar_config_safa(Paquete *paquete)
{
	memcpy(&quantum, paquete->Payload, sizeof(u_int32_t));
	int len = 0;
	memcpy(&len, paquete->Payload + sizeof(u_int32_t), sizeof(u_int32_t));
	algoritmo = malloc(len + 1);
	memcpy(algoritmo, paquete->Payload + sizeof(u_int32_t) * 2, len);
	algoritmo[len] = '\0';
}

void handshake(int socket, int emisor)
{
	Mensaje *mensaje = malloc(sizeof(Mensaje));
	mensaje->socket = socket;
	mensaje->paquete.header.tipoMensaje = ESHANDSHAKE;
	mensaje->paquete.header.tamPayload = 0;
	mensaje->paquete.header.emisor = emisor;
	enviar_mensaje(*mensaje);
}

void leer_config(char *path)
{
	config = config_create("/home/utnso/tp-2018-2c-Nene-Malloc/cpu/src/CPU.config");
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
	char *puerto = config_get_string_value(config, "PUERTO_SAFA");
	char *ip = config_get_string_value(config, "IP_SAFA");
	return connect_to_server(ip, puerto);
}

int crear_socket_dam()
{
	char *puerto = config_get_string_value(config, "PUERTO_DIEGO");
	char *ip = config_get_string_value(config, "IP_DIEGO");
	return connect_to_server(ip, puerto);
}

int crear_socket_fm9()
{
	char *puerto = config_get_string_value(config, "PUERTO_FM9");
	char *ip = config_get_string_value(config, "IP_FM9");
	return connect_to_server(ip, puerto);
}

//PRIMITIVAS

int ejecutar_abrir(char **parameters, DTB *dtb)
{
	printf("ABRIR\n");
	char *path = parameters[1];

	ArchivoAbierto *archivo_encontrado = _DTB_encontrar_archivo(dtb, path);

	if (archivo_encontrado != NULL)
		return 0;

	Paquete *pedido_a_dam = malloc(sizeof(Paquete));
	pedido_a_dam->header.tipoMensaje = ABRIR;
	pedido_a_dam->header.emisor = CPU;
	int len = 0;
	void *path_serializado = string_serializar(path, &len);
	pedido_a_dam->header.tamPayload = len + sizeof(u_int32_t);
	pedido_a_dam->Payload = malloc(pedido_a_dam->header.tamPayload);
	memcpy(pedido_a_dam->Payload, &dtb->gdtPID, sizeof(u_int32_t));
	memcpy(pedido_a_dam->Payload + sizeof(u_int32_t), path_serializado, len);
	//mensaje a DAM
	EnviarPaquete(socket_dam, pedido_a_dam);

	dtb->entrada_salidas++;
	bloqueate_safa(dtb);

	//Cuando se realiza esta operatoria, el CPU desaloja al DTB indicando a S-AFA
	//que el mismo está esperando que El Diego cargue en FM9 el archivo deseado.
	//Esta operación hace que S-AFA bloquee el G.DT.

	//Cuando El Diego termine la operatoria de cargarlo en FM9,
	//avisará a S-AFA los datos requeridos para obtenerlo de FM9
	//para que pueda desbloquear el G.DT.

	return 1;
}

int ejecutar_concentrar(char **parametros, DTB *dtb)
{
	printf("CONCENTRAR\n");
	sleep(retardo);
	return 0;
}

int ejecutar_asignar(char **parametros, DTB *dtb)
{
	printf("ASIGNAR\n");
	char *path = parametros[1];
	int linea = atoi(parametros[2]);
	char *dato = parametros[3];

	log_info(logger, "dato a asignar:%s", dato);
	ArchivoAbierto *archivo_encontrado = _DTB_encontrar_archivo(dtb, path);

	if (archivo_encontrado == NULL)
		return enviar_pid_y_pc(dtb, ABORTARA);
		//Error 20001: El archivo no se encuentra abierto

	Paquete *datos_a_fm9 = malloc(sizeof(Paquete));

	int desplazamiento = 0;
	int len_int = INTSIZE * 2; //pid y linea
	int size_archivo = 0;
	int size_dato = 0;
	void *arch_serializado = DTB_serializar_archivo(archivo_encontrado, &size_archivo);
	void *dato_serializado = string_serializar(dato, &size_dato);

	datos_a_fm9->Payload = malloc(len_int + size_archivo + size_dato);

	memcpy(datos_a_fm9->Payload, &dtb->gdtPID, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(datos_a_fm9->Payload + desplazamiento, &linea, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(datos_a_fm9->Payload + desplazamiento, arch_serializado, size_archivo);
	desplazamiento += size_archivo;
	memcpy(datos_a_fm9->Payload + desplazamiento, dato_serializado, size_dato);
	desplazamiento += size_dato;

	//mando a FM9 el pid, la linea (offset), la estructura archivo abierto y el dato

	datos_a_fm9->header = cargar_header(desplazamiento, ASIGNAR, CPU);

	EnviarPaquete(socket_fm9, datos_a_fm9);
	free(arch_serializado);
	free(path);
	free(dato_serializado);
	free(dato);
	free(datos_a_fm9->Payload);
	free(datos_a_fm9);

	Paquete asigno_fm9;
	RecibirPaqueteCliente(socket_fm9, &asigno_fm9);

	if (asigno_fm9.header.tipoMensaje != ASIGNAR)
	{
		log_debug(logger, "Fallo asignar, Error %d", asigno_fm9.header.tipoMensaje);
		Paquete error_asignar;
		int tam_pid_y_pc = 0;
		error_asignar.Payload = serializar_pid_y_pc(dtb->gdtPID, dtb->PC, &tam_pid_y_pc);
		error_asignar.header = cargar_header(tam_pid_y_pc, asigno_fm9.header.tipoMensaje, CPU);
		EnviarPaquete(socket_safa, &error_asignar);
		free(error_asignar.Payload);
		return 1;
	}
	return 0;
}

void enviar_pedido_recurso(char *recurso, int pid, int pc, Tipo senial)
{
	int len_recurso = 0;
	void *recurso_serializado = string_serializar(recurso, &len_recurso);

	Paquete pedido_recurso;
	pedido_recurso.Payload = malloc(len_recurso + INTSIZE * 2);
	memcpy(pedido_recurso.Payload, recurso_serializado, len_recurso);
	memcpy(pedido_recurso.Payload + len_recurso, &pid, INTSIZE);
	memcpy(pedido_recurso.Payload + len_recurso + INTSIZE, &pc, INTSIZE);

	pedido_recurso.header = cargar_header(len_recurso + INTSIZE * 2, senial, CPU);

	EnviarPaquete(socket_safa, &pedido_recurso);
	log_debug(logger, "El pid %d pidio %d para el recurso %s", pid, senial, recurso);
	free(recurso_serializado);
	free(pedido_recurso.Payload);
}

int ejecutar_wait(char **parametros, DTB *dtb)
{
	printf("WAIT\n");

	char *recurso = parametros[1];

	enviar_pedido_recurso(recurso, dtb->gdtPID, dtb->PC, WAIT);

	log_debug(logger, "Queda bloqueado esperando que lo habilite el otro hilo");
	sem_wait(&sem_senial);

	//si no se pudo asignar, mando mensaje de bloqueo
	if (senial_safa == ROJADIRECTA)
	{
		bloqueate_safa(dtb);
		return 1;
	}
	return 0;
}

int ejecutar_signal(char **parametros, DTB *dtb)
{
	printf("SIGNAL\n");
	char *recurso = parametros[1];

	enviar_pedido_recurso(recurso, dtb->gdtPID, dtb->PC, SIGNAL);

	log_debug(logger, "Queda bloqueado esperando que lo habilite el otro hilo");
	return 0;
}

/**
 * Verificará que el archivo solicitado esté abierto por el G.DT.
 * En caso que no se encuentre, se abortará el G.DT.
 * Se enviará una solicitud a El Diego indicando que se requiere hacer
 * un Flush del archivo, enviando los parámetros necesarios para que 
 * pueda obtenerlo desde FM9 y guardarlo en MDJ.
 * TODO: Se comunicará al proceso S-AFA que el G.DT se encuentra a la espera
 * de una respuesta por parte de El Diego y S-AFA lo bloqueará.
 */
int ejecutar_flush(char **parametros, DTB *dtb)
{
	printf("FLUSH\n");

	char *path = parametros[1];

	ArchivoAbierto *archivo_encontrado = _DTB_encontrar_archivo(dtb, path);

	if (archivo_encontrado == NULL)
		return enviar_pid_y_pc(dtb, ABORTARF); // Error 30001: El archivo no se encuentra abierto

	//mensaje al DAM
	int desplazamiento = 0;
	void *archivo_serializado = DTB_serializar_archivo(archivo_encontrado, &desplazamiento);
	Paquete *flush_a_dam = malloc(sizeof(Paquete));
	flush_a_dam->header.emisor = CPU;
	flush_a_dam->header.tipoMensaje = FLUSH;
	flush_a_dam->header.tamPayload = INTSIZE + desplazamiento;
	flush_a_dam->Payload = malloc(flush_a_dam->header.tamPayload);

	memcpy(flush_a_dam->Payload, &dtb->gdtPID, INTSIZE);
	memcpy(flush_a_dam->Payload + INTSIZE, archivo_serializado, desplazamiento);
	EnviarPaquete(socket_dam, flush_a_dam);
	log_debug(logger, "Enviar flush a dam de pid %d", dtb->gdtPID);

	//mensaje a SAFA
	dtb->entrada_salidas++;
	bloqueate_safa(dtb);

	free(archivo_encontrado);
	free(flush_a_dam->Payload);
	free(flush_a_dam);
	return 1;
}

int ejecutar_close(char **parametros, DTB *dtb)
{
	printf("CLOSE\n");

	char *path = parametros[1];

	ArchivoAbierto *archivo_encontrado = _DTB_encontrar_archivo(dtb, path);

	if (archivo_encontrado == NULL)
		return enviar_pid_y_pc(dtb, ABORTARC); //Error 40001: El archivo no se encuentra abierto

	Paquete *liberar_memoria = malloc(sizeof(Paquete));

	int size_archivo = 0;
	int desplazamiento = 0;
	void *arch_serializado = DTB_serializar_archivo(archivo_encontrado, &size_archivo);

	liberar_memoria->Payload = malloc(INTSIZE + size_archivo);
	memcpy(liberar_memoria->Payload, &dtb->gdtPID, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(liberar_memoria->Payload + desplazamiento, arch_serializado, size_archivo);
	desplazamiento += size_archivo;

	//mando a FM9 el pid y la estructura archivo abierto serializada

	liberar_memoria->header = cargar_header(desplazamiento, CLOSE, CPU);

	EnviarPaquete(socket_fm9, liberar_memoria);

	Paquete archivo_cerrado;
	RecibirPaqueteCliente(socket_fm9, &archivo_cerrado);

	if (archivo_cerrado.header.tipoMensaje == FALLO_DE_SEGMENTO_CLOSE)
	{
		log_debug(logger, "Fallo de segmento close");

		Paquete error_close;
		int tam_pid_y_pc = 0;
		error_close.Payload = serializar_pid_y_pc(dtb->gdtPID, dtb->PC + 1, &tam_pid_y_pc);
		error_close.header = cargar_header(tam_pid_y_pc, archivo_cerrado.header.tipoMensaje, CPU);
		EnviarPaquete(socket_safa, &error_close);

		free(error_close.Payload);
		free(liberar_memoria->Payload);
		free(liberar_memoria);
		return 1;
	}

	EnviarPaquete(socket_safa, liberar_memoria);
	free(liberar_memoria->Payload);
	free(liberar_memoria);
	return 0;
	/*
	Verificará que el archivo solicitado esté abierto por el G.DT. 
	En caso que no se encuentre, se abortará el G.DT.
	Se enviará a FM9 la solicitud para liberar la memoria del archivo deseado.
 	TODO: Se liberará de las estructuras administrativas del DTB la referencia
	a dicho archivo.
	*/
}

int ejecutar_crear(char **parametros, DTB *dtb)
{
	printf("CREAR\n");

	char *path = parametros[1];
	int lineas = atoi(parametros[2]);

	int len = 0;
	void *path_serializado = string_serializar(path, &len);

	Paquete *crear_archivo = malloc(sizeof(Paquete));
	crear_archivo->header.tamPayload = len + INTSIZE * 2;
	crear_archivo->Payload = malloc(crear_archivo->header.tamPayload);

	int desplazamiento = 0;

	memcpy(crear_archivo->Payload, &dtb->gdtPID, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(crear_archivo->Payload + desplazamiento, path_serializado, len);
	desplazamiento += len;
	memcpy(crear_archivo->Payload + desplazamiento, &lineas, INTSIZE);

	crear_archivo->header = cargar_header(crear_archivo->header.tamPayload, CREAR_ARCHIVO, CPU);
	//mensaje a DAM
	EnviarPaquete(socket_dam, crear_archivo);
	log_debug(logger, "El pid %d quiere crear %s con %d lineas", dtb->gdtPID, path, lineas);

	//mensaje a SAFA
	dtb->entrada_salidas++;
	bloqueate_safa(dtb);

	/*
	Se deberá enviar a El Diego el archivo a crear
	con la cantidad de líneas necesarias. 
	El CPU desalojará el programa G.DT y S-AFA lo bloqueará.
	*/
	return 1;
}

int ejecutar_borrar(char **parametros, DTB *dtb)
{
	printf("BORRAR\n");
	char *path = parametros[1];

	int len = 0;
	void *path_serializado = string_serializar(path, &len);

	Paquete *borrar_archivo = malloc(sizeof(Paquete));
	borrar_archivo->header.emisor = CPU;
	borrar_archivo->header.tipoMensaje = BORRAR_ARCHIVO;
	borrar_archivo->header.tamPayload = INTSIZE + len;
	borrar_archivo->Payload = malloc(borrar_archivo->header.tamPayload);

	memcpy(borrar_archivo->Payload, &dtb->gdtPID, INTSIZE);
	memcpy(borrar_archivo->Payload + INTSIZE, path_serializado, len);
	//mensaje a DAM
	EnviarPaquete(socket_dam, borrar_archivo);

	//mensaje a SAFA
	dtb->entrada_salidas++;
	bloqueate_safa(dtb);

	return 1;
	/*Se deberá enviar a El Diego el archivo a borrar 
	y se desalojará el programa G.DT para bloquearlo.
	*/
}

void bloqueate_safa(DTB *dtb)
{
	if (!strcmp(algoritmo, "VRR"))
	{
		return;
	}
	Paquete *bloqueate_safa = malloc(sizeof(Paquete));
	bloqueate_safa->header.tipoMensaje = DTB_BLOQUEAR;
	bloqueate_safa->header.emisor = CPU;
	int tam_pid_y_pc = 0;
	void *pid_pc_serializados;
	pid_pc_serializados = serializar_pid_y_pc(dtb->gdtPID, dtb->PC, &tam_pid_y_pc);
	bloqueate_safa->header.tamPayload = tam_pid_y_pc + sizeof(u_int32_t);
	bloqueate_safa->Payload = malloc(bloqueate_safa->header.tamPayload);
	memcpy(bloqueate_safa->Payload, pid_pc_serializados, tam_pid_y_pc);
	memcpy(bloqueate_safa->Payload + tam_pid_y_pc, &dtb->entrada_salidas, sizeof(u_int32_t));
	EnviarPaquete(socket_safa, bloqueate_safa);
	log_debug(logger, "Le pido a safa que bloque el pid %d", dtb->gdtPID);

	free(pid_pc_serializados);
	free(bloqueate_safa->Payload);
	free(bloqueate_safa);
}

int enviar_pid_y_pc(DTB *dtb, Tipo tipo_mensaje)
{
		Paquete *paquete = malloc(sizeof(Paquete));
		int tam_pid_y_pc = 0;
		paquete->Payload = serializar_pid_y_pc(dtb->gdtPID, dtb->PC, &tam_pid_y_pc);
		paquete->header = cargar_header(tam_pid_y_pc, tipo_mensaje, CPU);
		EnviarPaquete(socket_safa, paquete);
		log_header(logger, paquete, "Mando pid: %d y pc: %d a Safa", dtb->gdtPID, dtb->PC);
		free(paquete->Payload);
		free(paquete);
		dtb_liberar(dtb);

		return 1;
}

void dtb_liberar(DTB *dtb)
{
	list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
	free(dtb);
}