#ifndef PRIMITIVAS_H
#define PRIMITiVAS_H

#include <stdio.h>

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
}

/* A structure which contains information on the commands this program
   can understand. */
typedef void *(*DoRunTimeChecks)();

struct Primitiva{
        char *name; /* User printable name of the function. */
	DoRunTimeChecks func; /* Function to call to do the job. */
	char *doc; /* Documentation for this function.  */
};

struct Primitiva primitivas[] = {
	{"abrir", ejecutar_abrir, "El comando abrir permitirá abrir un nuevo archivo para el escriptorio."},
	{"concentrar", ejecutar_concentrar, "La primitiva concentrar será un comando de no operación. El objetivo del mismo es hacer correr una unidad de tiempo de quantum."},
	{"asignar", ejecutar_asignar, "La primitiva asignar permitirá la asignación de datos a una línea dentro de path pasado."},
	{"wait", ejecutar_wait, "La primitiva Wait permitirá la espera y retención de distintos recursos tanto existentes como no que administrá S-AFA."},
	{"signal", ejecutar_signal, "La primitiva Signal permitirá la liberación de un recurso que son administrados S-AFA."},
	{"flush", ejecutar_flush, "La primitiva Flush permite guardar el contenido de FM9 a MDJ."},
	{"close", ejecutar_close, "La primitiva Close permite liberar un archivo abierto que posea el G.DT."},
	{"crear", ejecutar_crear, "La primitiva crear permitirá crear un nuevo archivo en el MDJ. Para esto, se deberá utilizar de intermediario a El Diego."},
	{"borrar", ejecutar_borrar, "La primitiva borrar permitirá eliminar un archivo de MDJ."},
};

#endif