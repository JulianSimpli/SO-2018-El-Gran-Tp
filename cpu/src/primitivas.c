#include "primitivas.h"

void *ejecutar_abrir(char **parameters)
{
	printf("ABRIR\n");
	char *parameter = parameters[1];

	/*
	if (is_file_already_open(parameter)) {
		return true;
	}
	*/

	//En caso que no se encuentre abierto, se enviará una solicitud a El Diego
	//para que traiga desde el MDJ el archivo requerido.

	//send_message(, ELDIEGO);

	//Cuando se realiza esta operatoria, el CPU desaloja al DTB indicando a S-AFA
	//que el mismo está esperando que El Diego cargue en FM9 el archivo deseado.
	//Esta operación hace que S-AFA bloquee el G.DT.

	//send_message(, SAFA);

	//Cuando El Diego termine la operatoria de cargarlo en FM9,
	//avisará a S-AFA los datos requeridos para obtenerlo de FM9
	//para que pueda desbloquear el G.DT.

	return false;
}

void *ejecutar_concentrar(char **parametros)
{
	printf("CONCENTRAR\n");
}

void *ejecutar_asignar(char *linea)
{
	printf("ASIGNAR\n");
}

void *ejecutar_wait(char *linea)
{
	printf("WAIT\n");
}

void *ejecutar_signal(char *linea)
{
	printf("SIGNAL\n");
}

void *ejecutar_flush(char *linea)
{
	printf("FLUSH\n");
}

void *ejecutar_close(char *linea)
{
	printf("CLOSE\n");
}

void *ejecutar_crear(char *linea)
{
	printf("CREAR\n");
}

void *ejecutar_borrar(char *linea)
{
	printf("BORRAR\n");
