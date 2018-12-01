#include "helper.h"
#include <string.h>

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

