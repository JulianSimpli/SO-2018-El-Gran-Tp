#ifndef PARSER_H_
#define PARSER_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <commons/string.h>

	#define INICIO_KEYWORD "inicio"
	#define FIN_KEYWORD "fin"
	#define ABRIR_KEYWORD "abrir"
	#define CONCENTRAR_KEYWORD "concentrar"
	#define ASIGNAR_KEYWORD "asignar"
	#define WAIT_KEYWORD "wait"
	#define SIGNAL_KEYWORD "signal"
	#define FLUSH_KEYWORD "flush"
	#define CLOSE_KEYWORD "close"
	#define CREAR_KEYWORD "crear"
	#define BORRAR_KEYWORD "borrar"

	typedef struct {
		enum {
			INICIO,
			FIN,
			ABRIR,
			CONCENTRAR,
			ASIGNAR,
			WAIT,
			SIGNAL,
			FLUSH,
			CLOSE,
			CREAR,
			BORRAR
		} keyword;
		union {
			struct {
				char* path;
			} ABRIR;
			struct {
				char* path;
				int nroLinea;
				char* datos;
			} ASIGNAR;
			struct {
				char* recurso;
			} WAIT;
			struct {
				char* recurso;
			} SIGNAL;
			struct {
				char* path;
			} FLUSH;
			struct {
				char* path;
			} CLOSE;
			struct {
				char* path;
				int cantLineas;
			} CREAR;
			struct {
				char* path;
			} BORRAR;
		} argumentos;
		char** _raw;
	} escriptorio_op;

	escriptorio_op parse(char* line);
	void destruir_operacion(escriptorio_op op);

#endif /* PARSER_H_ */
