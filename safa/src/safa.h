#ifndef SAFA_H_
#define SAFA_H_

#include "sockets.h"
#include "planificador.h"

//Declaracion de constantes
#define EJECUTAR "ejecutar"
#define STATUS "status"
#define FINALIZAR "finalizar"
#define METRICAS "metricas"

//Declaracion de variables globales
char *IP, *ALGORITMO_PLANIFICACION;
int PUERTO, QUANTUM, MULTIPROGRAMACION, RETARDO_PLANIF;

t_list* listaHilos;
t_list* lista_cpu;

bool end;

pthread_mutex_t mutex_handshake_diego;
pthread_mutex_t mutex_handshake_cpu;

t_log* logger;

// Declaracion de funciones
void crearLogger();
void inicializarVariables();
void obtenerValoresArchivoConfiguracion();
void imprimirArchivoConfiguracion();
void consola();
void parseoConsola(char* operacion, char* primerParametro);
void accion(void* socket);

#endif /* SAFA_H_ */