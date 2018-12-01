#ifndef SOCKETS_H_
#define SOCKETS_H_

#include "helper.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10 // Cuántas conexiones pendientes se mantienen en cola del server
#define TAMANIOHEADER sizeof(Header)
#define STRHANDSHAKE "10"

char* Emisores[5];

typedef enum {CPU, FM9, ELDIEGO, MDJ, SAFA} Emisor;

typedef enum {
	ESHANDSHAKE, ESSTRING, ESDATOS, SUCCESS, ERROR,				        // Mensajes generales
	VALIDAR_ARCHIVO, CREAR_ARCHIVO, OBTENER_DATOS, GUARDAR_DATOS, BORRAR_ARCHIVO,   // Emisor: DIEGO, Receptor: MDJ
	NUEVA_PRIMITIVA, CLOSE, ASIGNAR,							// Emisor: CPU, Receptor:FM9
	DTB_EJECUTO, DTB_BLOQUEAR, PROCESS_TIMEOUT, QUANTUM_FALTANTE, WAIT, SIGNAL,	// Emisor: CPU, Receptor: SAFA
	DUMMY_SUCCES, DUMMY_FAIL, DTB_SUCCES, DTB_FAIL,	DTB_FINALIZAR,			// Emisor: Diego, Receptor: SAFA
	ESDTBDUMMY, ESDTB, ESDTBQUANTUM, FINALIZAR, CAMBIO_CONFIG, ROJADIRECTA, SIGASIGA,		// Emisor: SAFA, Receptor: CPU
	FIN_EJECUTANDO,
	FIN_BLOQUEADO,									// Emisor: SAFA, Receptor: DIEGO
	ABRIR, FLUSH,									 //Emisor: DIEGO, Receptor: FM9
	LINEA_PEDIDA,									//Emisor: FM9, Receptor: CPU
	ABORTAR=20001, 										//Errores ASIGNAR
	ABORTARF=30001,										//Errores FLUSH
	ABORTARC=40001,										//Errores CLOSE
	CARGADO,									//Emisor: FM9, Receptor: DIEGO
	ARCHIVO,									//Emisor: MDJ, Receptor: DIEGO
	} Tipo;													

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
void EnviarHandshakeELDIEGO(int socketFD);
void RecibirHandshake(int socketFD, Emisor emisor);
int RecibirPaqueteServidor(int socketFD, Emisor receptor, Paquete* paquete); //Responde al recibir un Handshake
int RecibirPaqueteCliente(int socketFD, Paquete* paquete); //No responde los Handshakes
int RecibirPaqueteServidorSafa(int socketFD, Emisor receptor, Paquete* paquete);
int RecibirPaqueteServidorFm9(int socketFD, Emisor receptor, Paquete* paquete);
int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir);
Header cargar_header(int tamanio_payload, Tipo tipo_mensaje, Emisor emisor);

/*
Para entender como funciona S-AFA, leer en orden:
ServidorConcurrente -> StartServidor -> RecibirPaqueteServidorSafa -> RecibirDatos


Funcion: ServidorConcurrente

Parametros:
- char* ip, int puerto: IP y Puerto de escucha. Sirven para crear el servidor (ver funcion StartServidor).
- Emisor nombre: Nombre del servidor. Cada proceso tiene asociada una constante en sockets.h
- t_list** listaDeHilos: Puntero a (puntero a) lista de hilos.
- bool* terminar: Puntero a booleano. Sirve para hacer while (1) mientras conectan procesos.
- void (*accionHilo)(void* socketFD)): Accion es la especifica de cada hilo. SocketFD es el parametro que recibe accion. Ver ejemplo abajo.

Descripcion:
Comienza un servidor asociandole una lista de hilos de conexiones.
Cada vez que un cliente se conecta al servidor, acepta la conexion, crea un hilo de ejecucion y lo agrega a la lista de hilos.
Cada hilo tiene asociado un socket de esa conexion especifica y una accion a realizar.
Por ultimo, cierra el socket y cuando termina la ejecucion de un hilo, saca ese elemento de memoria.

Ejemplo de uso:
	ServidorConcurrente(IP, PUERTO, SAFA, &listaHilos, &end, accion);
S-AFA la usa para recibir conexiones de distintas CPU y de DAM (El Diego).
La accion del hilo esta en la funcion accion y recibe paquetes (ver funcion RecibirPaqueteServidorSafa)
y los decodifica segun emisor primero, y segun tipo de mensaje despues.
Con el emisor y el tipo de mensaje, sabe que hacer especificamente.
*/

/*
Funcion: int StartServidor

Parametros: char*MyIP, int MyPort

Descripcion: Crea Servidor. Devuelve socket de escucha listo para aceptar conexiones.
*/

/*
Funcion: int RecibirPaqueteServidorSafa
(es igual a RecibirPaqueteServidor con 1 adaptacion a nuestro sistema)

Parametros: int socketFD, Emisor receptor, Paquete* paquete

Descripcion: Recibe paquetes de un socket (ver RecibirDatos). Retorna lo mismo que recv()

Ejemplo de uso:
La usa accion de S-AFA: 
- while (RecibirPaqueteServidorSafa(socketFD, SAFA, &paquete) > 0)
Va a ser >0 siempre que haya un paquete a recibir.

- Llena el header de un paquete - RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
Si header.tipoMensaje == handshake realiza el handshake con EnviarHandshake(socketFD, receptor);
- Llena el payload de un paquete - RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
*/

/*
Funcion: int RecibirDatos

Parametros: void* paquete, int socketFD, uint32_t cantARecibir

Descripcion: Llena el paquete haciendo recv() hasta que lo recibido iguale cantARecibir.

Posibles valores de retorno (igual que recv):
- Tamanio de lo ultimo recv (>0) si lo recibido == cantARecibir;
- <0 si hubo un error en el recv.
- 0 si se desconecto el cliente.

Ejemplo de uso:
La usa RecibirPaqueteServidorSafa:
- Llena el header de un paquete - RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
- Llena el payload de un paquete - RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
*/

/*
Funcion: EnviarHandshake

Parametros: int socketFD, Emisor emisor

Descripcion: Crea paquete cargado con los datos de un handshake(*) y lo envia con EnviarPaquete(SocketFD, paquete).

 (*)Datos del handshake asumen tamPayload en 0.
*/

/*
Funcion: bool EnviarPaquete

Parametros: int socketCliente, Paquete* paquete

Descripcion: A partir de un paquete, agrega el payload al buffer a enviar si no fue handshake (capaz cambiar)
Hace el send mientras hasta que termine de llenar todo el tamanio del paquete

Valores de retorno: True si send se hizo bien. False si send genero error (send devuelve -1 en ese caso).
*/

/*
Funcion: bool EnviarMensaje

Parametros: int socketFD, char* msg, Emisor emisor

Descripcion: Genera el paquete con header.tipoDeMensaje = ESSTRING y lo envia con EnviarPaquete.

Valores de retorno: True si el send se hizo bien y False si send genero error (igual que EnviarPaquete)
*/

#endif //SOCKETS_H_
