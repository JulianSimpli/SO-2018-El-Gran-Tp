#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "helper.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10 // Cuántas conexiones pendientes se mantienen en cola del server
#define TAMANIOHEADER sizeof(Header)
#define STRHANDSHAKE "10"

typedef enum { CPU, FM9, ELDIEGO, MDJ, SAFA } Emisor;

//Emisores:
//#define CPU "CPU    "
//#define FM9 "FM9    "
//#define ELDIEGO "ELDIEGO"
//#define MDJ "MDJ    "
//#define SAFA "SAFA   "

//typedef enum { ESHANDSHAKE, ESSTRING, ESDATOS } Tipo;
typedef enum { ESHANDSHAKE, ESSTRING, ESDATOS, SUCCESS, ERROR,
	VALIDAR_ARCHIVO, CREAR_ARCHIVO, OBTENER_DATOS, GUARDAR_DATOS, BORRAR_ARCHIVO} Tipo;

typedef struct {
	Tipo tipoMensaje;
	uint32_t tamPayload;
	Emisor emisor;
}__attribute__((packed)) Header;

typedef struct {
	Header header;
	void* Payload;
}__attribute__((packed)) Paquete;

typedef struct {
	pthread_t hilo;
	int socket;
} structHilo;

typedef struct {
	Paquete paquete;
	int socket;
}__attribute__((packed)) Mensaje;

//typedef struct {
//	pid_t pid;
//	int socket;
//} structProceso;

//void Servidor(char* ip, int puerto, char nombre[8],
//		void (*accion)(Paquete* paquete, int socketFD),
//		int (*RecibirPaquete)(int socketFD, char receptor[8], Paquete* paquete));
//void ServidorConcurrenteForks(char* ip, int puerto, char nombre[8], t_list** listaDeProcesos,
//		bool* terminar, void (*accionPadre)(void* socketFD), void (*accionHijo)(void* socketFD));
void ServidorConcurrente(char* ip, int puerto, Emisor nombre, t_list** listaDeHilos,
		bool* terminar, void (*accionHilo)(void* socketFD));
int StartServidor(char* MyIP, int MyPort);
int ConectarAServidor(int puertoAConectar, char* ipAConectar, Emisor servidor, Emisor cliente,
		void RecibirElHandshake(int socketFD, Emisor emisor));
int ConectarAServidorCpu(int puertoAConectar, char* ipAConectar, Emisor servidor, Emisor cliente,
		void RecibirElHandshake(int socketFD, Emisor emisor), void EnviarElHandshake(int socketFD, Emisor emisor));

bool EnviarDatos(int socketFD, Emisor emisor, void* datos, int tamDatos);
bool EnviarDatosTipo(int socketFD, Emisor emisor, void* datos, int tamDatos, Tipo tipoMensaje);
bool EnviarMensaje(int socketFD, char* msg, Emisor emisor);
bool EnviarPaquete(int socketCliente, Paquete* paquete);
void EnviarHandshake(int socketFD, Emisor emisor);
void RecibirHandshake(int socketFD, Emisor emisor);
int RecibirPaqueteServidor(int socketFD, Emisor receptor, Paquete* paquete); //Responde al recibir un Handshake
int RecibirPaqueteCliente(int socketFD, Emisor receptor, Paquete* paquete); //No responde los Handshakes
int RecibirPaqueteServidorSafa(int socketFD, Emisor receptor, Paquete* paquete);
int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir);

#endif //SOCKETS_H_
