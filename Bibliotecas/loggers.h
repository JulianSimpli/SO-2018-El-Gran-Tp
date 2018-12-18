#ifndef LOGGERS_H_
#define LOGGERS_H_

#include "dtb.h"
#include "sockets.h"
#include "helper.h"
#include <commons/string.h>

t_log *logger;

void log_header(t_log *logger, Paquete *paquete, const char *contexto, ...);
void log_dtb(t_log *logger, DTB *dtb, const char* _contexto, ...);
void log_archivo(t_log *logger, ArchivoAbierto *archivo, const char *_contexto, ...);
void log_posicion(t_log *logger, Posicion *posicion, const char *_context, ...);
void loggear_contexto(t_log *logger, const char *_contexto, ...);
char *devolver_tipo(Tipo tipo_mensaje);
void log_strings_test1(t_log *logger, const char *strings, ...);
void log_strings_test2(t_log *logger, const char *context, ...);

#endif // LOGGERS_H_