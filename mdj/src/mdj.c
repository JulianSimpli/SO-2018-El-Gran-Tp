#include "mdj.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>

void *interpretar_consola(void *args);

int main(int argc, char **argv)
{
	inicializar(argv);

	pthread_t dam;
	pthread_create(&dam, NULL, atender_dam, NULL);

	pthread_t consola;
	pthread_create(&consola, NULL, interpretar_consola, NULL);

	pthread_join(dam, NULL);
	pthread_join(consola, NULL);
	return 0;
}

void *interpretar_consola(void *args)
{
	char *linea;
	while (1)
	{
		char *PS1 = malloc(strlen(current_path) + 2);
		strcpy(PS1, current_path);
		linea = readline(strcat(PS1, "$"));
		if (linea)
			add_history(linea);
		interpretar(linea);
		free(linea);
	}
}

void *atender_dam()
{
	//Hace el handshake y empieza a recibir paquetes
	handshake_dam();
	log_info(logger, "Se concreto el handshake con DAM, empiezo a recibir paquetes");
	Paquete paquete;

	while (recibir_paquete(socket_dam, &paquete) > 0)
	{
		pthread_t p_thread;
		pthread_create(&p_thread, NULL, atender_peticion, &paquete);
	}
}

void *atender_peticion(void *args)
{
	Paquete *paquete = (Paquete *)args;
	log_debug(logger, "Tamanio %d", paquete->header.tamPayload);
	//esto deberia usar semaforos porque ahora admite peticiones concurrentes y pueden querer escribir dos procesos el mismo arhivo
	interpretar_paquete(paquete);
}

void leer_config(char *path)
{
	config = config_create("/home/utnso/tp-2018-2c-Nene-Malloc/mdj/src/MDJ.config");
	retardo = config_get_int_value(config, "RETARDO") / 1000;
	char *mnt_config = config_get_string_value(config, "PUNTO_MONTAJE");
	mnt_path = malloc(strlen(mnt_config) + 1);
	strcpy(mnt_path, mnt_config);
	file_path = malloc(strlen(mnt_config) + strlen("Archivos") + 1);
	blocks_path = malloc(strlen(mnt_config) + strlen("Bloques") + 1);
	metadata_path = malloc(strlen(mnt_config) + strlen("Metadata") + 1);
	current_path = malloc(strlen(mnt_config) + strlen("Archivos") + 1);
	strcpy(file_path, mnt_path);
	strcpy(blocks_path, mnt_path);
	strcpy(metadata_path, mnt_path);
	strcpy(current_path, mnt_path);
	strcat(file_path, "Archivos");
	strcat(blocks_path, "Bloques");
	strcat(metadata_path, "Metadata");
	strcat(current_path, "Archivos");
}

void inicializar_log(char *program)
{
	logger = log_create("MDJ.log", program, true, LOG_LEVEL_DEBUG);
}

void inicializar_semaforos()
{
	sem_init(&sem_io, 0, 1);
}

void inicializar(char **argv)
{
	inicializar_log(argv[0]);
	log_info(logger, "Log cargado");
	leer_config(argv[1]);
	log_info(logger, "Config cargada");
	cargar_bitarray();
	log_info(logger, "Bitarray cargado");
	inicializar_semaforos();
}

int interpretar(char *linea)
{
	int i = 0, existe = 0;
	//Calcula la cantidad de comandos que hay en el array
	size_t command_size = sizeof(commands) / sizeof(commands[0]) - 1;

	char **parametros = string_split(linea, " ");
	//Deberia parsear la linea, separar la primer palabra antes del espacio del resto
	//Para poder identificar el comando y que el resto son parametros necesarios para que ejecute
	//Por ej: linea = cd /home/utnso/
	//parsear_linea(linea, &comando, &parametro);
	//comando = cd
	//parametro = /home/utnso

	for (i; i < command_size; i++)
	{
		if (!strcmp(commands[i].name, parametros[0]))
		{
			existe = 1;
			log_info(logger, commands[i].doc);
			//llama a la funcion que tiene guardado ese comando en la estructura
			commands[i].func(parametros);
			break;
		}
	}

	if (!existe)
	{
		error_show("No existe el comando %s\n", linea);
	}

	return 0;
}

int obtener_size_escriptorio(char *path)
{
	char *ruta = ruta_absoluta(path);
	t_config *metadata = config_create(ruta);
	int size = config_get_int_value(metadata, "TAMANIO");
	config_destroy(metadata);
	free(ruta);
	return size;
}

void interpretar_paquete(Paquete *paquete)
{
	Paquete *respuesta = malloc(sizeof(Paquete));
	respuesta->header = cargar_header(0, SUCCESS, MDJ);

	int accion = paquete->header.tipoMensaje;
	sleep(retardo);
	sem_wait(&sem_io);

	switch (accion)
	{
	case VALIDAR_ARCHIVO:
		log_info(logger, "Validar archivo");

		int desplazamiento = 0;
		char *prueba = string_deserializar(paquete->Payload, &desplazamiento);
		char *search_file = malloc(paquete->header.tamPayload + 1);
		strcpy(search_file, prueba);

		int existe = validar_archivo(search_file, file_path);

		if (existe == 0)
		{
			enviar_error(PATH_INEXISTENTE);
			return;
		}

		int tamanio = obtener_size_escriptorio(prueba);

		log_debug(logger, "%s pesa %d bytes", ruta_absoluta, tamanio);

		Paquete respuesta;
		respuesta.header = cargar_header(INTSIZE, SUCCESS, MDJ);
		respuesta.Payload = malloc(INTSIZE);
		memcpy(respuesta.Payload, &tamanio, INTSIZE);
		EnviarPaquete(socket_dam, &respuesta);
		break;
	case CREAR_ARCHIVO:
		log_info(logger, "Crear archivo");
		crear_archivo(paquete);
		break;
	case OBTENER_DATOS:
		log_info(logger, "Obtener datos");
		obtener_datos(paquete);
		break;
	case GUARDAR_DATOS:
		log_info(logger, "Guardar datos");
		guardar_datos(paquete);
		break;
	case BORRAR_ARCHIVO:
		log_info(logger, "Borrar archivos");
		borrar_archivo(paquete);
		break;
	default:
		log_warning(logger, "La accion %d no tiene respuesta posible", accion);
		break;
	}

	sem_post(&sem_io);
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

//Calcula size segun tamanio de linea que paso fm9
t_list *get_space(int size)
{
	int blocks = calcular_bloques(size);
	log_debug(logger, "Necesita %d bloques", blocks);
	return conseguir_lista_de_bloques(blocks);
}

int file_size(char *path)
{
	FILE *f = fopen(path, "rb");
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	fclose(f);
	return size;
}

void enviar_error(Tipo tipo)
{
	Paquete error;
	error.header = cargar_header(0, tipo, MDJ);
	log_error(logger, "Error %d", tipo);
	EnviarPaquete(socket_dam, &error);
}

//TODO: Controlar el tamanio que tiene el paquete en la segunda call
//TODO: Liberar los que son punteros al finalizar cada recursividad
int validar_archivo(char *search_file, char *current_path)
{
	char current[2000];
	DIR *dir;
	struct dirent *ent;
	// char *current = malloc(strlen(current_path) + 1);
	// strcpy(current, current_path);
	log_debug(logger, "esta es mi ruta completa a buscar: %s", search_file);

	char **directorios = string_split(search_file, "/");
	char *route = directorios[0];
	log_debug(logger, "estoy parado en: %s", current_path);
	log_debug(logger, "nuevo acceso a %s", route);

	dir = opendir(current_path);
	if (dir == NULL)
	{
		log_info(logger, "No pude abrir este directorio: %s", current_path);
		return 0;
	}

	log_info(logger, "Entre al dir: %s", current_path);
	while ((ent = readdir(dir)) != NULL)
	{
		//Verificar con strcmp si ya existe
		log_info(logger, "Busco: %s", route);
		if (strcmp(route, ent->d_name) == 0)
		{
			if (ent->d_type == DT_DIR)
			{
				int position = strlen(route) + 1;
				char *ruta_parcial = string_substring_from(search_file, position);
				log_debug(logger, "sub es %s", ruta_parcial);
				strcpy(current, current_path);
				strcat(current, "/");
				strcat(current, ent->d_name);
				if (validar_archivo(ruta_parcial, current) > 0)
				{
					// closedir(dir);
					return 1;
				}
			}
			else
			{
				char *archivo = malloc(strlen(ent->d_name) + 1);
				strcpy(archivo, ent->d_name);
				log_info(logger, "Encontré el archivo %s", archivo);
				closedir(dir);
				return 1;
			}
		}
	}
	closedir(dir);
	log_info(logger, "No encontré el archivo %s", search_file);
	return 0;
}

int contar_ocurrencias(char *cadena, char caracter)
{
	int count = 0;
	for (int i = 0; cadena[i] != '\0'; i++)
	{
		if (cadena[i] = caracter)
			count++;
	}
}

void crear_archivo(Paquete *paquete)
{
	// STRTOK CON /
	// Valido si existe directorio, cuando no exista creo directorio nuevo
	// Cuando llego al ultimo char* de lo que strtokie, creo el archivo
	DIR *dir;
	struct dirent *ent;
	int offset = 0;
	char *ruta = string_deserializar(paquete->Payload, &offset);
	int bytes_a_crear = 0;
	memcpy(&bytes_a_crear, paquete->Payload + offset, sizeof(u_int32_t));

	char *directorio_actual = malloc(strlen(file_path) + 1);
	strcpy(directorio_actual, file_path);
	char **directorios = string_split(ruta, "/");

	int accesos = contar_ocurrencias(ruta, '/');

	for (int i = 0; i < accesos - 1; i++)
	{
		char *directorio_a_buscar = directorios[i];
		directorio_actual = realloc(directorio_actual, strlen(directorio_actual) + strlen(directorio_a_buscar) + 1);
		strcat(directorio_actual, "/");
		strcat(directorio_actual, directorio_a_buscar);
		dir = opendir(directorio_actual);
		if (dir == NULL)
		{
			log_info(logger, "Procedo a crear este directorio: %s", current_path);
			mkdir(directorio_a_buscar, 0777);
		}
		closedir(dir);
	}

	//Devuelve lista de bloques libres o lista empty
	t_list *bloques_libres = get_space(bytes_a_crear);

	//Valida que tenga espacio suficiente
	if (list_size(bloques_libres) == 0)
		enviar_error(ESPACIO_INSUFICIENTE_CREAR);

	//for que recorrra la lista de bloques libres, llame a la funcion create_block y marque el bitarray
	//Va decrementando la cantidad de lineas que le resten escribir con \n necesarios en c/u
	int retorno = crear_bloques(bloques_libres, &bytes_a_crear);

	Archivo_metadata nuevo_archivo;
	nuevo_archivo.nombre = ruta_absoluta(ruta);
	nuevo_archivo.bloques = bloques_libres;
	nuevo_archivo.tamanio = bytes_a_crear;

	//Crea el archivo metadata con la informacion de donde estan los bloques
	//TODO: Agregar en una lista que este archivo esta siendo leido
	//TODO: Si esta abierto hace un wait en ese semaforo y queda bloqueado
	//Caso de carrera en el que un escriptorio crea un archivo que otro proceso va a leer
	int resultado = guardar_metadata(&nuevo_archivo);

	if (resultado != 0)
		enviar_error(ESPACIO_INSUFICIENTE_CREAR);

	Paquete respuesta;
	respuesta.header = cargar_header(0, SUCCESS, MDJ);
	EnviarPaquete(socket_dam, &respuesta);
	free(nuevo_archivo.nombre);
}

char *ruta_absoluta(char *ruta)
{
	char *path = malloc(strlen(file_path) + strlen(ruta) + 1);
	strcpy(path, file_path);
	strcat(path, ruta);
	return path;
}

void obtener_datos(Paquete *paquete)
{
	log_debug(logger, "Recibi el payload de tamanio %d", paquete->header.tamPayload);

	int offset = 0;
	int size = sizeof(u_int32_t);
	char *ruta = string_deserializar(paquete->Payload, &offset);
	int bytes_offset = 0;
	memcpy(&bytes_offset, paquete->Payload + offset, size);
	offset += size;
	int bytes_a_devolver = 0;
	memcpy(&bytes_a_devolver, paquete->Payload + offset, size);
	offset += size;
	int final = bytes_a_devolver;

	log_debug(logger, "Obtener %s %d %d", ruta, bytes_offset, bytes_a_devolver);

	//Calcular en que bloque cae segun el offset y cuantos bloques va leer segun el size
	//Lee metadata del archivo pasado por el path
	char *path = ruta_absoluta(ruta);
	log_debug(logger, "Abro la metadata de %s", path);
	t_config *metadata = config_create(path);
	char **bloques_ocupados = config_get_array_value(metadata, "BLOQUES");

	int bloque_inicial = bytes_offset / tamanio_bloques;
	int offset_interno = bytes_offset % tamanio_bloques;
	int leido = 0;

	//Crear paquete de payload del tamanio leido y cargar el contenido de los bloques con un for
	char *buffer = malloc(bytes_a_devolver + 1);

	for (int i = bloque_inicial; bytes_a_devolver > 0; i++)
	{
		int bloque = atoi(bloques_ocupados[i]);
		char *ubicacion = get_block_full_path(bloque);
		log_debug(logger, "Abro el bloque que esta en %s", ubicacion);
		FILE *bloque_abierto = fopen(ubicacion, "rb");
		fseek(bloque_abierto, offset_interno, SEEK_SET);
		int leer = tamanio_bloques - offset_interno;
		//bytes_a_devolver < tamanio_bloques - offset_interno, leo la cantidad de bytes_a_devolver
		//sino, lee la resta y pasa al siguiente archivo

		if (bytes_a_devolver < leer)
			leer = bytes_a_devolver;

		log_debug(logger, "Leo %d", leer);
		char *aux = malloc(leer);
		fread(aux, 1, leer, bloque_abierto);
		memcpy(buffer + leido, aux, leer);
		fclose(bloque_abierto);
		log_debug(logger, "bytes_a_devolver %d", bytes_a_devolver);
		bytes_a_devolver -= leer;
		leido += leer;
		free(aux);
	}

	buffer[final] = '\0';
	log_debug(logger, "EL contenido es \n%s", buffer);

	//Es el caso que el archivo no se pudo leer por alguna razon
	if (buffer == NULL)
		enviar_error(ERROR);

	Paquete respuesta;
	int desplazamiento = 0;
	void *serializado = string_serializar(buffer, &desplazamiento);
	log_debug(logger, "La respuesta pesa %d", desplazamiento);
	respuesta.header = cargar_header(desplazamiento, SUCCESS, MDJ);
	respuesta.Payload = serializado;

	EnviarPaquete(socket_dam, &respuesta);
	config_destroy(metadata);
	free(respuesta.Payload);
	free(buffer);
}

void guardar_datos(Paquete *paquete)
{
	Paquete respuesta;
	int offset = 0;
	int size = sizeof(u_int32_t);
	char *ruta = string_deserializar(paquete->Payload, &offset);
	//TODO: Agregar en una lista que este archivo esta siendo leido
	//TODO: Si esta abierto hace un wait en ese semaforo y queda bloqueado
	int bytes_offset = 0;
	memcpy(&bytes_offset, paquete->Payload + offset, size);
	offset += size;
	int buffer_size = 0;
	memcpy(&buffer_size, paquete->Payload + offset, size);
	offset += size;
	char *datos_a_guardar = string_deserializar(paquete->Payload, &offset);

	int tamanio = validar_archivo(ruta, file_path);

	if (tamanio == 0)
	{
		enviar_error(ARCHIVO_NO_EXISTE_FLUSH);
		return;
	}

	t_config *metadata = config_create(ruta);
	char **bloques_ocupados = config_get_array_value(metadata, "BLOQUES");

	int bloque_inicial = bytes_offset / tamanio_bloques;
	int offset_interno = bytes_offset % tamanio_bloques;

	int cantidad_bloques_del_path = config_get_int_value(metadata, "TAMANIO") / tamanio_bloques;

	for (int i = bloque_inicial; buffer_size > 0 && i < cantidad_bloques_del_path; i++)
	{
		int bloque = atoi(bloques_ocupados[i]);
		char *ubicacion = get_block_full_path(bloque);
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
			int bloque = atoi((char *)list_get(nuevos_bloques, i));
			char *ubicacion = get_block_full_path(bloque);
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

	respuesta.header.tipoMensaje = SUCCESS;

	//TODO: Validar archivo y reponder error si no existe
	//TODO: Lee metadata del archivo pasado por el path
	//TODO: Calcular en que bloque cae segun el offset y cuantos bloques va escribir segun el size
	//TODO: Ir consumiendo el buffer segun tamanio de bloque
	//TODO: Crear paquete OK
	EnviarPaquete(socket_dam, &respuesta);
}
