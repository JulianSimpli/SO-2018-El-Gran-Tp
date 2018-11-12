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
int PUERTO, RETARDO_PLANIF, QUANTUM;

t_list *lista_hilos;

bool end;

sem_t mutex_handshake_diego;
sem_t mutex_handshake_cpu;

// Declaracion de funciones
void crear_logger();
void inicializar_variables();
void crear_listas();
void llenar_lista_estados();
void obtener_valores_archivo_configuracion();
void imprimir_archivo_configuracion();
void consola();
void parseo_consola(char* operacion, char* primerParametro);
void accion(void* socket);
void manejar_paquetes_diego(Paquete *paquete, int socketFD);
void manejar_paquetes_CPU(Paquete *paquete, int socketFD);
void *handshake_cpu_serializar(int *tamanio_payload);
void enviar_handshake_cpu(int socketFD);
void enviar_handshake_diego(int socketFD);

#endif /* SAFA_H_ */

