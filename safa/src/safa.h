#ifndef SAFA_H_
#define SAFA_H_

#include "planificador.h"

//Declaracion de constantes
#define EJECUTAR "ejecutar"
#define STATUS "status"
#define FINALIZAR "finalizar"
#define METRICAS "metricas"

//Declaracion de variables globales
char *IP, *ALGORITMO_PLANIFICACION;
int PUERTO, QUANTUM, RETARDO_PLANIF;

t_list* lista_hilos;
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
void manejar_paquetes_CPU(Paquete* paquete, int* socketFD);

#endif /* SAFA_H_ */
