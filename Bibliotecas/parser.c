#include "parser.h"
#include <commons/string.h>

escriptorio_op parse(char* line){

	char* auxLine = string_duplicate(line);
	string_trim(&auxLine);
	char** lineaSpliteada = string_n_split(auxLine, 4, " ");

	char* keyword = lineaSpliteada[0];
	escriptorio_op retorno = {
			._raw = lineaSpliteada
	};

	if (!strcmp(keyword, INICIO)) {
		retorno.keyword = INICIO;
	} else if (string_starts_with(keyword, FIN)) {
		retorno.keyword = FIN;
	} else if (!strcmp(keyword, ABRIR)) {
		retorno.keyword = ABRIR;
		retorno.argumentos.ABRIR.path = lineaSpliteada[1];
	} else if (!strcmp(keyword, ASIGNAR)){
		int nroLinea = atoi(lineaSpliteada[2]);
		retorno.keyword = ASIGNAR;
		retorno.argumentos.ASIGNAR.path = lineaSpliteada[1];
		retorno.argumentos.ASIGNAR.nroLinea = nroLinea;
		retorno.argumentos.ASIGNAR.datos = lineaSpliteada[3];
	} else if (!strcmp(keyword, WAIT)) {
		retorno.keyword = WAIT;
		retorno.argumentos.WAIT.recurso = lineaSpliteada[1];
	} else if (!strcmp(keyword, SIGNAL)) {
		retorno.keyword = SIGNAL;
		retorno.argumentos.SIGNAL.recurso = lineaSpliteada[1];
	} else if (!strcmp(keyword, FLUSH)) {
		retorno.keyword = FLUSH;
		retorno.argumentos.FLUSH.path = lineaSpliteada[1];
	} else if (!strcmp(keyword, CLOSE)) {
		retorno.keyword = CLOSE;
		retorno.argumentos.CLOSE.path = lineaSpliteada[1];
	} else if (!strcmp(keyword, CREAR)) {
		int cantLineas = atoi(lineaSpliteada[2]);
		retorno.keyword = CREAR;
		retorno.argumentos.CREAR.path = lineaSpliteada[1];
		retorno.argumentos.CREAR.cantLineas = cantLineas;
	} else if (!strcmp(keyword, BORRAR)) {
		retorno.keyword = BORRAR;
		retorno.argumentos.BORRAR.path = lineaSpliteada[1];
	}

	free(auxLine);

	return retorno;
}

void destruir_operacion(escriptorio_op operacion){
	if(operacion._raw){
		string_iterate_lines(operacion._raw, (void*) free);
		free(operacion._raw);
	}
}
