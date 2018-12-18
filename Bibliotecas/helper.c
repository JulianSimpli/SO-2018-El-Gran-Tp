#include "helper.h"

char* integer_to_string(char*string,int x) {
	string = malloc(10);
	if (string) {
		sprintf(string, "%d", x);
	}
	string=realloc(string,strlen(string)+1);
	return string; // caller is expected to invoke free() on this buffer to release memory
}

size_t getFileSize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;
}

void *string_serializar(char *string, int *desplazamiento)
{
	u_int32_t len_string = strlen(string);
	void *serializado = malloc(sizeof(len_string) + len_string);

	memcpy(serializado + *desplazamiento, &len_string, sizeof(len_string));
	*desplazamiento += sizeof(len_string);
	memcpy(serializado + *desplazamiento, string, len_string);
	*desplazamiento += len_string;

	return serializado;
}

char *string_deserializar(void *data, int *desplazamiento)
{
	u_int32_t len_string = 0;
	memcpy(&len_string, data + *desplazamiento, sizeof(len_string));
	*desplazamiento += sizeof(len_string);

	char *string = malloc(len_string + 1);
	memcpy(string, data + *desplazamiento, len_string);
	*(string + len_string) = '\0';
	*desplazamiento = *desplazamiento + len_string;

	return string;
}

void *serializar_pid_y_pc(u_int32_t pid, u_int32_t pc, int *tam_pid_y_pc)
{
	void *payload = malloc(sizeof(u_int32_t) * 2);

	memcpy(payload + *tam_pid_y_pc, &pid, sizeof(u_int32_t));
	*tam_pid_y_pc += sizeof(u_int32_t);
	memcpy(payload + *tam_pid_y_pc, &pc, sizeof(u_int32_t));
	*tam_pid_y_pc += sizeof(u_int32_t);

	return payload;
}

void deserializar_pid_y_pc(void *payload, u_int32_t *pid, u_int32_t *pc, int *desplazamiento)
{
	int tamanio = sizeof(u_int32_t);
	memcpy(pid, payload + *desplazamiento, tamanio);
	*desplazamiento += tamanio;
	memcpy(pc, payload + *desplazamiento, tamanio);
	*desplazamiento += tamanio;
	log_debug(logger, "Deserialice: PID: %d y PC: %d", *pid, *pc);
}

void *serializar_posicion(Posicion *posicion, int *tam_posicion)
{
	void *payload = malloc(INTSIZE * 4);
	int desplazamiento = 0;

	memcpy(payload + desplazamiento, &posicion->pid, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(payload + desplazamiento, &posicion->segmento, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(payload + desplazamiento, &posicion->pagina, INTSIZE);
	desplazamiento += INTSIZE;
	memcpy(payload + desplazamiento, &posicion->offset, INTSIZE);
	desplazamiento += INTSIZE;

	*tam_posicion = desplazamiento;
	return payload;
}

Posicion *deserializar_posicion(void *payload, int *desplazamiento)
{
	Posicion *posicion = malloc(sizeof(Posicion));

	memcpy(&posicion->pid, payload + *desplazamiento, INTSIZE);
	*desplazamiento += INTSIZE;
	memcpy(&posicion->segmento, payload + *desplazamiento, INTSIZE);
	*desplazamiento += INTSIZE;
	memcpy(&posicion->pagina, payload + *desplazamiento, INTSIZE);
	*desplazamiento += INTSIZE;
	memcpy(&posicion->offset, payload + *desplazamiento, INTSIZE);
	*desplazamiento += INTSIZE;

	return posicion;
}

Posicion *generar_posicion(DTB *dtb, ArchivoAbierto *archivo, u_int32_t offset)
{
	Posicion *posicion = malloc(sizeof(Posicion));

	posicion->pid = dtb->gdtPID;
	posicion->segmento = archivo->segmento;
	posicion->pagina = archivo->pagina;
	if(offset)
		posicion->offset = offset - 1;
	else
		posicion->offset = dtb->PC - 1;
	
	return posicion;
}

//funciones de exit

void _exit_with_error(int socket, char *error_msg, void *buffer)
{
	if (buffer != NULL)
	{
		free(buffer);
	}
	log_error(logger, error_msg);
	close(socket);
	exit_gracefully(1);
}

void exit_gracefully(int return_nr)
{
	log_destroy(logger);
	exit(return_nr);
}
