#include "cpu.h"
#include <commons/string.h>

int quantum;
char *algoritmo;
int socket_dam;
int socket_safa;
int socket_fm9;
int retardo;

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
	handshake_dam();
	//handshake_fm9();
	pthread_t p_thread_one;
	pthread_create(&p_thread_one, NULL, hilo_safa, NULL);
	pthread_t p_thread_two;
	pthread_create(&p_thread_two, NULL, hilo_safa, NULL);

	pthread_join(p_thread_one, NULL);
	pthread_join(p_thread_two, NULL);
	return 0;
}

//
void *hilo_safa()
{
	while (1)
	{
		Paquete *paquete = malloc(sizeof(Paquete));
		RecibirPaqueteCliente(socket_safa, paquete);
		interpretar_safa(paquete);
	}
}

int ejecutar_quantum(Paquete *paquete)
{
	DTB *dtb = DTB_deserializar(paquete);
	int i;
	// cambios = 1;
	for (i = 0; i < quantum; i++)
	{
		// sem_wait(cambios);
		char *primitiva = pedir_primitiva(dtb);
		ejecutar(primitiva, dtb); //avanzar el PC dentro del paquete
		dtb->PC++;
		if (finalizar)
			break;
		// sem_post(cambios);
	}

	if (finalizar)
	{
		Paquete *paquete_a_diego = malloc(sizeof(Paquete));
		paquete_a_diego->header.tipoMensaje = FINALIZAR;
		paquete_a_diego->header.tamPayload = 0;
		paquete_a_diego->header.emisor = CPU;
		//empezar a mandar mensajes de finalizacion al DAM
		//un paquete con el tipoDeMensaje = FINALIZAR
		//emisor:CPU
		//tamPayload: 0
	}

	//armar el paquete para mandar a safa, que dependiendo de si es RR o VRR, envía quantum restante
	//si es RR, manda DTB_EJECUTO
	//si es VRR, manda QUANTUM_FALTANTE
	Paquete *nuevo_paquete = malloc(sizeof(Paquete));
	nuevo_paquete->header.tipoMensaje = DTB_EJECUTO;
	void *DTB_serializado = DTB_serializar(dtb, &nuevo_paquete->header.tamPayload);
	nuevo_paquete->header.emisor = CPU;
	nuevo_paquete->Payload = DTB_serializado;

	if (!strcmp(algoritmo, "VRR"))
	{
		nuevo_paquete->header.tipoMensaje = QUANTUM_FALTANTE;
		nuevo_paquete->Payload = realloc(nuevo_paquete->Payload, nuevo_paquete->header.tamPayload + sizeof(u_int32_t));
		int restante = quantum - i;
		memcpy(nuevo_paquete->Payload + nuevo_paquete->header.tamPayload, &restante, sizeof(u_int32_t));
		nuevo_paquete->header.tamPayload += sizeof(u_int32_t);
	}
	EnviarPaquete(socket_safa, nuevo_paquete);
}

char *pedir_primitiva(DTB *dtb)
{
	//solo le envio a FM9 el PID y el PC del dtb que me llegó
	Paquete *paquete = malloc(sizeof(Paquete));
	int len = sizeof(u_int32_t) * 2;
	paquete->header.tamPayload = len;
	paquete->header.tipoMensaje = NUEVA_PRIMITIVA;
	paquete->header.emisor = CPU;
	paquete->Payload = malloc(len);
	memcpy(paquete->Payload, &dtb->gdtPID, sizeof(u_int32_t));
	memcpy(paquete->Payload + sizeof(u_int32_t), &dtb->PC, sizeof(u_int32_t));
	// pedido_de_primitiva->header.tamPayload = 0;
	int ret = EnviarPaquete(socket_fm9, paquete);
	if (ret != 1)
	{
		Paquete *primitiva_recibida = malloc(sizeof(Paquete));
		RecibirPaqueteCliente(socket_fm9, primitiva_recibida);
		char *primitiva = malloc(primitiva_recibida->header.tamPayload + 1);
		memcpy(primitiva, primitiva_recibida->Payload, primitiva_recibida->header.tamPayload);
		primitiva[primitiva_recibida->header.tamPayload] = '\0';
		return primitiva;
	}
}

int interpretar_safa(Paquete *paquete)
{
	switch (paquete->header.tipoMensaje)
	{
	case ESDTBDUMMY:
		log_debug(logger, "ESDTBDUMMY");
		EnviarPaquete(socket_dam, paquete); //envia el dtb al diego para que este haga el pedido correspondiente a MDJ
		break;
	case ESDTB:
		log_debug(logger, "ESDTBDUMMY");
		ejecutar_quantum(paquete);
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
}

void handshake_dam()
{
	socket_dam = crear_socket_dam();
	EnviarHandshake(socket_dam, CPU);
	Paquete *paquete = malloc(sizeof(Paquete));
	RecibirPaqueteCliente(socket_dam, paquete);
	if (paquete->header.tipoMensaje != ESHANDSHAKE)
		_exit_with_error(socket_dam, "No se logro el handshake", NULL);

	log_info(logger, "Se concreto el handshake con DAM, empiezo a recibir mensajes");
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
	hints.ai_family = AF_UNSPEC;	 // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
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

int ejecutar(char *linea, DTB *dtb)
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
			primitivas[i].func(parameters, dtb);
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

//PRIMITIVAS

void *ejecutar_abrir(char **parameters, DTB *dtb)
{
	printf("ABRIR\n");
	char *path = parameters[1];

	ArchivoAbierto *archivo_encontrado = _DTB_encontrar_archivo(dtb, path);

	if (archivo_encontrado != NULL)
		return NULL;

	Paquete *pedido_a_dam = malloc(sizeof(Paquete));
	pedido_a_dam->header.tipoMensaje = ABRIR;
	pedido_a_dam->header.emisor = CPU;
	int len = 0;
	void *path_serializado = string_serializar(path, &len);
	pedido_a_dam->header.tamPayload = len + sizeof(u_int32_t);
	pedido_a_dam->Payload = malloc(pedido_a_dam->header.tamPayload);
	memcpy(pedido_a_dam->Payload, &dtb->gdtPID, sizeof(u_int32_t));
	memcpy(pedido_a_dam->Payload + sizeof(u_int32_t), path_serializado, len);

	EnviarPaquete(socket_dam, pedido_a_dam);
	Paquete *bloqueate_safa = malloc(sizeof(Paquete));
	bloqueate_safa->header.tipoMensaje = DTB_BLOQUEAR;
	bloqueate_safa->header.emisor = CPU;
	bloqueate_safa->header.tamPayload = 0;
	EnviarPaquete(socket_safa, bloqueate_safa);

	//Cuando se realiza esta operatoria, el CPU desaloja al DTB indicando a S-AFA
	//que el mismo está esperando que El Diego cargue en FM9 el archivo deseado.
	//Esta operación hace que S-AFA bloquee el G.DT.

	//Cuando El Diego termine la operatoria de cargarlo en FM9,
	//avisará a S-AFA los datos requeridos para obtenerlo de FM9
	//para que pueda desbloquear el G.DT.

	return NULL;
}

void *ejecutar_concentrar(char **parametros, DTB *dtb)
{
	printf("CONCENTRAR\n");
	sleep(retardo);
	return NULL;
}

void *ejecutar_asignar(char **parametros, DTB *dtb)
{
	printf("ASIGNAR\n");
	char *path = parametros[1];
	int linea = atoi(parametros[2]);
	char *dato = parametros[3];

	ArchivoAbierto *archivo_encontrado = _DTB_encontrar_archivo(dtb, path);

	if (archivo_encontrado == NULL)
	{
		Paquete *abortar_gdt = malloc(sizeof(Paquete));
		abortar_gdt->header.tamPayload = 0;
		abortar_gdt->header.emisor = CPU;
		abortar_gdt->header.tipoMensaje = ABORTAR;

		EnviarPaquete(socket_safa, abortar_gdt);
		//mandar mensaje a SAFA
		//abortar GDT
		//Error 20001: El archivo no se encuentra abierto
	}
	else
	{
		Paquete *datos_a_fm9 = malloc(sizeof(Paquete));
		datos_a_fm9->header.emisor = CPU;
		datos_a_fm9->header.tipoMensaje = ASIGNAR;
		int len_path = 0;
		int len_dato = 0;
		void *path_serializado = string_serializar(path, &len_path);
		void *dato_serializado = string_serializar(dato, &len_dato);
		datos_a_fm9->header.tamPayload = len_path + len_dato + sizeof(u_int32_t);
		datos_a_fm9->Payload = malloc(datos_a_fm9->header.tamPayload);
		memcpy(datos_a_fm9->Payload, path_serializado, len_path);
		memcpy(datos_a_fm9->Payload + len_path, &linea, sizeof(u_int32_t));
		memcpy(datos_a_fm9->Payload + len_path + sizeof(u_int32_t), dato_serializado, len_dato);

		EnviarPaquete(socket_fm9, datos_a_fm9);
		//mandar mensaje a FM9
		//enviar parametros. El string sin splitear? o separados?
	}
	return NULL;
}

void *ejecutar_wait(char *linea, DTB *dtb)
{
	printf("WAIT\n");
}

void *ejecutar_signal(char *linea, DTB *dtb)
{
	printf("SIGNAL\n");
}

void *ejecutar_flush(char *linea, DTB *dtb)
{
	printf("FLUSH\n");
}

void *ejecutar_close(char *linea, DTB *dtb)
{
	printf("CLOSE\n");
}

void *ejecutar_crear(char *linea, DTB *dtb)
{
	printf("CREAR\n");
}

void *ejecutar_borrar(char *linea, DTB *dtb)
{
	printf("BORRAR\n");
}
