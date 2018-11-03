#include "mdj.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>

int main(int argc, char **argv)
{
	inicializar(argv);

	pthread_t p_thread;
	pthread_create(&p_thread, NULL, atender_dam, NULL);

	Mensaje *misLadrillos = malloc(sizeof(Mensaje));
	misLadrillos->paquete.Payload = malloc(strlen("/torneo/equipos/Racing.txt") + 1);
	strcpy(misLadrillos->paquete.Payload, "/torneo/equipos/Racing.txt");
	validar_archivo(misLadrillos, file_path);
	//t_config *metadata = crear_archivo("/home/daniel/FileSystemMock/Archivos/alumnos/alumno.bin");
	//marcarbitarray(metadata);
	log_info(logger, "Marco el bitarray");

	char *linea;
	while (1)
	{
		linea = readline("MDJ>");
		if (linea)
			add_history(linea);
		interpretar(linea);
		free(linea);
	}
	return 0;
}

void *atender_dam()
{
	int conexion = escuchar_conexiones();

	//Hace el handshake y empieza a recibir mensajes
	Mensaje *mensaje = recibir_mensaje(conexion);

	if (mensaje->paquete.header.tipoMensaje != ESHANDSHAKE)
	{
		_exit_with_error(conexion, "No se logro el handshake", NULL);
	}

	handshake(conexion, MDJ);
	log_info(logger, "Se concreto el handshake con DAM, empiezo a recibir mensajes");

	while (1)
	{
		Mensaje *mensaje = recibir_mensaje(conexion);
		pthread_t p_thread;
		pthread_create(&p_thread, NULL, atender_peticion, mensaje);
	}
}

void *atender_peticion(void *mensaje)
{
	//esto deberia usar semaforos porque ahora admite peticiones concurrentes y pueden querer escribir dos procesos el mismo arhivo
	Mensaje *respuesta = interpretar_mensaje(mensaje);
	log_info(logger, "Respuesta %d", respuesta->paquete.header.tipoMensaje);
	enviar_mensaje(*respuesta);
}

void leer_config(char *path)
{
	config = config_create(strcat(path, "/MDJ.config"));
	char *mnt_config = config_get_string_value(config, "PUNTO_MONTAJE");
	mnt_path = malloc(strlen(mnt_config) + 1);
	strcpy(mnt_path, mnt_config);
	file_path = malloc(strlen(mnt_config) + strlen("Archivos") + 1);
	strcpy(file_path, mnt_path);
	strcat(file_path, "Archivos");
}

void inicializar_log(char *program)
{
	logger = log_create("MDJ.log", program, true, LOG_LEVEL_DEBUG);
}

void crear_bitarray()
{
	t_config *bitarray_metadata = config_create("/home/daniel/FileSystemMock/Metadata/Metadata.bin");
	int cantidad_bloques = config_get_int_value(bitarray_metadata, "CANTIDAD_BLOQUES");
	config_destroy(bitarray_metadata);
	bitarray = bitarray_create_with_mode("/home/daniel/FileSystemMock/Metadata/bitarray.bin", cantidad_bloques / 8, LSB_FIRST);
	bitarray_set_bit(bitarray, 1);
	size_t n = bitarray_get_max_bit(bitarray);
	log_info(logger, "El bitarray tiene %d bits", n);
}

void inicializar(char **argv)
{
	inicializar_log(argv[0]);
	log_info(logger, "Log cargado exitosamente");
	leer_config(argv[1]);
	log_info(logger, "Config cargada exitosamente");
	// crear_bitarray();
	// log_info(logger, "bitarray creado exitosamente");
}

void retardo()
{
	int retardo = config_get_int_value(config, "RETARDO") / 1000;
	sleep(retardo);
}

int interpretar(char *linea)
{
	int i = 0, existe = 0;
	//Calcula la cantidad de comandos que hay en el array
	size_t command_size = sizeof(commands) / sizeof(commands[0]) - 1;

	//Deberia parsear la linea, separar la primer palabra antes del espacio del resto
	//Para poder identificar el comando y que el resto son parametros necesarios para que ejecute
	//Por ej: linea = cd /home/utnso/
	//parsear_linea(linea, &comando, &parametro);
	//comando = cd
	//parametro = /home/utnso

	for (i; i < command_size; i++)
	{
		if (!strcmp(commands[i].name, linea))
		{
			existe = 1;
			log_info(logger, commands[i].doc);
			//llama a la funcion que tiene guardado ese comando en la estructura
			commands[i].func(linea);
			break;
		}
	}

	if (!existe)
	{
		error_show("No existe el comando %s\n", linea);
	}

	return 0;
}

int escuchar_conexiones()
{

	char *port = config_get_string_value(config, "PUERTO");
	char *ip = "127.0.0.1";

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

	log_info(logger, "Empiezo a escuchar el socket");

	int conexion = listen(server_socket, 1);

	if (conexion < 0)
	{
		_exit_with_error(server_socket, "Fallo el listen", NULL);
		exit_gracefully(1);
	}

	//int conexion_aceptada = accept(server_socket, server_info->ai_addr, server_info->ai_addrlen);
	int conexion_aceptada = accept(server_socket, NULL, NULL);

	if (conexion_aceptada < 0)
	{
		_exit_with_error(server_socket, "Fallo el accept", NULL);
		exit_gracefully(1);
	}

	log_info(logger, "Conexion aceptada nuevo socket: %d", conexion_aceptada);

	close(server_socket);
	return conexion_aceptada;
}

Mensaje *interpretar_mensaje(Mensaje *mensaje)
{
	Mensaje *respuesta = (Mensaje *)malloc(sizeof(Mensaje));
	respuesta->paquete.header.emisor = MDJ;
	//TODO: Poner destinatario DAM

	int accion = mensaje->paquete.header.tipoMensaje;

	switch (accion)
	{
	case VALIDAR_ARCHIVO:
		log_info(logger, "Validar archivo");
		//TODO: Preguntar si mdj es el unico que llama a este proceso
		respuesta->paquete.Payload = (void*)(__intptr_t)(mensaje, file_path);
		//TODO: Cambiar firma de validar_archivo
		break;
	case CREAR_ARCHIVO:
		log_info(logger, "Crear archivo");
		crear_archivo(mensaje);
		break;
	case OBTENER_DATOS:
		log_info(logger, "Obtener datos");
		obtener_datos(mensaje);
		break;
	case GUARDAR_DATOS:
		log_info(logger, "Guardar datos");
		guardar_datos(mensaje);
		break;
	case BORRAR_ARCHIVO:
		log_info(logger, "Borrar archivos");

		char **parametros = string_split(mensaje->paquete.Payload, " ");
		char *ruta = parametros[1];

		//Primero verifica que el archivo todavia no exista
		int existe = validar_archivo(mensaje, file_path);

		if (existe)
		{
			respuesta->paquete.header.tamPayload = sizeof(uint32_t);
			respuesta->paquete.Payload = (void *)10001;
			break;
		}
		t_config *metadata = config_create(ruta);
		char **bloques_ocupados = config_get_array_value(metadata, "BLOQUES");

		int cantidad_bloques_del_path = config_get_int_value(metadata, "TAMANIO") / tamanio_bloques;

		for (int i = 0; i < cantidad_bloques_del_path; i++)
		{
		}
		//TODO: Validar archivo y tirar error si no existe
		//TODO: Lee metadata del archivo y devuelve los bloques
		//TODO: Modifica el bitarray
		//TODO: Borra los .bin
		//TODO: Borra los metadata
		//TODO: Crear mensaje OK
		break;
	default:
		log_warning(logger, "La accion %d no tiene respuesta posible", accion);
		break;
	}

	return respuesta;
}

int calcular_bloques(int tamanio)
{
	int bloques = tamanio / tamanio_bloques;
	return tamanio % tamanio_bloques == 0 ? bloques : bloques + 1;
}

t_list *conseguir_lista_de_bloques(int bloques)
{
	t_list *free_blocks = list_create();
	int i;
	for (i = 1; bloques != 0 && i < cantidad_bloques; i++)
	{
		//El bitarray esta desocupado en esa posicion
		if (bitarray_test_bit(bitarray, i))
		{
			log_debug(logger, "Bloque libre: %d", i);
			list_add(free_blocks, (void *)(__intptr_t)i);
			bloques--;
		}
	}
	if (bloques != 0)
		return list_create(); //Returns empty list

	return free_blocks;
}

t_list *get_space(int size)
{
	int blocks = calcular_bloques(size);
	log_debug(logger, "Necesita %d bloques", blocks);
	return conseguir_lista_de_bloques(blocks);
}

int validar_archivo(Mensaje *mensaje, char *current_path)
{
	DIR *dir;
	struct dirent *ent;
	char *search_file = string_duplicate(mensaje->paquete.Payload);
	char *route = strtok(search_file, "/");
	log_info(logger, "Busco: %s", route);

	dir = opendir(current_path);
	log_info(logger, "Entro al dir: %s", current_path);

	while ((ent = readdir(dir)) != NULL)
	{
		if (strcmp(route, ent->d_name) == 0)
		{
			if (ent->d_type == DT_DIR)
			{
				int position = strlen(route) + 1;
				route = string_substring_from(mensaje->paquete.Payload, position);
				strcat(current_path, "/");
				strcat(current_path, ent->d_name);
				mensaje->paquete.Payload = route;
				if (validar_archivo(mensaje, current_path))
				{
					return 1;
				}
			}
			else
			{
				log_info(logger, "Encontré el archivo %s", ent->d_name);

				return 1;
			}
		}
		//Agarra el path del mensaje
		//Recorrer el directorio Archivos
		//Verificar con strcmp si ya existe
	}
	closedir(dir);
	log_info(logger, "No encontré el archivo %s", search_file);
	return 0;
}

void marcarbitarray(t_config *metadata)
{
	char **bloques = config_get_array_value(metadata, "BLOQUES");
	log_info(logger, "Ocupa el bloque %d", atoi(bloques[0]));
	log_info(logger, "Ocupa el bloque %d", atoi(bloques[1]));
	bitarray_set_bit(bitarray, 1);
	log_info(logger, "Ocupa el bloque %d", atoi(bloques[2]));
	bitarray_set_bit(bitarray, atoi(bloques[0]));
	bitarray_set_bit(bitarray, atoi(bloques[1]));
	log_info(logger, "Ocupa el bloque %d", atoi(bloques[3]));
	bitarray_set_bit(bitarray, atoi(bloques[2]));
	bitarray_set_bit(bitarray, atoi(bloques[3]));
}

void crear_archivo(Mensaje *mensaje)
{
	Mensaje *respuesta = malloc(sizeof(Mensaje));
	char **parametros = string_split(mensaje->paquete.Payload, " ");
	char *ruta = parametros[1];
	int bytes_a_crear = atoi(parametros[2]);
	//Primero verifica que el archivo todavia no exista
	int existe = validar_archivo(mensaje, file_path);

	if (existe)
	{
		respuesta->paquete.header.tamPayload = sizeof(uint32_t);
		respuesta->paquete.Payload = (void *)50001;
	}

	//TODO: Calcular size segun tamanio de linea que paso fm9
	t_list *bloques_libres = get_space(bytes_a_crear);
	//Valida que tenga espacio suficiente
	//TODO: Devuelve lista de bloques libres o lista empty

	if (list_size(bloques_libres) == 0)
	{
		respuesta->paquete.header.tipoMensaje = ERROR;
		respuesta->paquete.Payload = (void *)50002;
	}
	int retorno = crear_bloques(bloques_libres, &bytes_a_crear);

	Archivo_metadata *nuevo_archivo = malloc(sizeof(Archivo_metadata));
	nuevo_archivo->nombre = malloc(strlen(ruta + 1));
	strcpy(nuevo_archivo->nombre, ruta);
	nuevo_archivo->bloques = bloques_libres;
	nuevo_archivo->tamanio = (__intptr_t)parametros[2];

	int resultado = guardar_metadata(nuevo_archivo);

	//for que recorrra la lista de bloques libres, llame a la funcion create_block y marque el bitarray
	//Va decrementando la cantidad de lineas que le resten escribir con \n necesarios en c/u

	//Crea el archivo metadata con la informacion de donde estan los bloques

	//Responde a dam un ok
	if (resultado == 0)
	{
		respuesta->paquete.header.tamPayload = 0;
		respuesta->paquete.header.tipoMensaje = SUCCESS;
	}
}

void obtener_datos(Mensaje *mensaje)
{
	Mensaje *respuesta = malloc(sizeof(Mensaje));
	char **parametros = string_split(mensaje->paquete.Payload, " ");
	char *ruta = parametros[1];
	int bytes_offset = atoi(parametros[2]);
	int bytes_a_devolver = atoi(parametros[3]);
	//Primero verifica que el archivo todavia no exista
	int existe = validar_archivo(mensaje, file_path);

	if (existe)
	{
		respuesta->paquete.header.tamPayload = sizeof(uint32_t);
		respuesta->paquete.Payload = (void *)10001;
	}
	//TODO: Validar archivo y reponder error si no existe
	//TODO: Lee metadata del archivo pasado por el path
	//TODO: Calcular en que bloque cae segun el offset y cuantos bloques va leer segun el size
	//TODO: Crear mensaje de payload del tamanio leido y cargar el contenido de los bloques con un for
	t_config *metadata = config_create(ruta);
	char **bloques_ocupados = config_get_array_value(metadata, "BLOQUES");

	int bloque_inicial = bytes_offset / tamanio_bloques;
	int offset_interno = bytes_offset % tamanio_bloques;

	char *buffer = malloc(bytes_a_devolver);

	for (int i = bloque_inicial; bytes_a_devolver > 0; i++)
	{
		int bloque_actual = atoi(bloques_ocupados[i]);
		char *ubicacion = get_block_full_path(bloque_actual);
		FILE *bloque_abierto = fopen(ubicacion, "r");
		fseek(bloque_abierto, offset_interno, SEEK_SET);
		offset_interno = 0;
		int leer = tamanio_bloques - offset_interno;
		//bytes_a_devolver < tamanio_bloques - offset_interno, leo la cantidad de bytes_a_devolver
		//sino, lee la resta y pasa al siguiente archivo

		if (bytes_a_devolver < leer)
			leer = bytes_a_devolver;
		fread(buffer, 1, leer, bloque_abierto);
		fclose(bloque_abierto);
	}

	if (buffer != NULL)
	{
		respuesta->paquete.header.tamPayload = strlen(buffer + 1);
		respuesta->paquete.Payload = buffer;
		respuesta->paquete.header.tipoMensaje = SUCCESS;
	}
}

void guardar_datos(Mensaje *mensaje)
{
	Mensaje *respuesta = malloc(sizeof(Mensaje));
	char **parametros = string_split(mensaje->paquete.Payload, " ");
	char *ruta = parametros[1];
	int bytes_offset = atoi(parametros[2]);
	int buffer_size = atoi(parametros[3]);
	char *datos_a_guardar = parametros[4];
	//Primero verifica que el archivo todavia no exista
	int existe = validar_archivo(mensaje, file_path);

	if (existe)
	{
		respuesta->paquete.header.tamPayload = sizeof(uint32_t);
		respuesta->paquete.Payload = (void *)10001;
	}
	t_config *metadata = config_create(ruta);
	char **bloques_ocupados = config_get_array_value(metadata, "BLOQUES");

	int bloque_inicial = bytes_offset / tamanio_bloques;
	int offset_interno = bytes_offset % tamanio_bloques;

	int cantidad_bloques_del_path = config_get_int_value(metadata, "TAMANIO") / tamanio_bloques;

	for (int i = bloque_inicial; buffer_size > 0 && i < cantidad_bloques_del_path; i++)
	{
		int bloque_actual = atoi(bloques_ocupados[i]);
		char *ubicacion = get_block_full_path(bloque_actual);
		FILE *bloque_abierto = fopen(ubicacion, "r+");
		fseek(bloque_abierto, offset_interno, SEEK_SET);
		int escribir = tamanio_bloques - offset_interno;
		offset_interno = 0;

		if (buffer_size < escribir)
			escribir = buffer_size;
		for (int j = 0; j < escribir; j++)
		{
			fwrite(datos_a_guardar, 1, escribir, bloque_abierto);
			datos_a_guardar = string_substring_from(datos_a_guardar, escribir);
			buffer_size -= escribir;
		}
		fclose(bloque_abierto);
	}

	if (buffer_size > 0)
	{
		t_list *nuevos_bloques = get_space(buffer_size);
		//por cada bloque nuevo, tengo que actualizar el tamanio de la metadata
		//actualizar el array de bloques
		int size = config_get_int_value(metadata, "TAMANIO") + buffer_size;
		char *tam = malloc(10);
		sprintf(tam, "%d", size);
		config_set_value(metadata, "TAMANIO", tam);

		t_list *bloques_viejos = list_create();
		for (int i = 0; i < cantidad_bloques_del_path; i++)
		{
			list_add(bloques_viejos, bloques_ocupados[i]);
		}

		list_add_all(bloques_viejos, nuevos_bloques);
		Archivo_metadata *archivo = list_get(bloques_viejos, 1);
		config_set_value(metadata, "BLOQUES", convertir_bloques_a_string(archivo));
		config_save(metadata);

		crear_bloques(nuevos_bloques, &buffer_size); //marco el bitarray

		for (int i = 0; buffer_size > 0; i++)
		{
			int bloque_actual = atoi(list_get(nuevos_bloques, i));
			char *ubicacion = get_block_full_path(bloque_actual);
			FILE *bloque_abierto = fopen(ubicacion, "r+");
			fseek(bloque_abierto, 0, SEEK_SET);
			int escribir = tamanio_bloques;

			if (buffer_size < escribir)
				escribir = buffer_size;
			for (int j = 0; j < escribir; j++)
			{
				fwrite(datos_a_guardar, 1, escribir, bloque_abierto);
				datos_a_guardar = string_substring_from(datos_a_guardar, escribir);
				buffer_size -= escribir;
			}
			fclose(bloque_abierto);
		}

		//escribir hasta terminar
	}
	//datos_a_guardar

	respuesta->paquete.header.tipoMensaje = SUCCESS;

	//TODO: Validar archivo y reponder error si no existe
	//TODO: Lee metadata del archivo pasado por el path
	//TODO: Calcular en que bloque cae segun el offset y cuantos bloques va escribir segun el size
	//TODO: Ir consumiendo el buffer segun tamanio de bloque
	//TODO: Crear mensaje OK
}