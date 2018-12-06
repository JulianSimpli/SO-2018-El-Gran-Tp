#include "sockets.h"
#include <commons/string.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
char* Emisores[5] = {"CPU", "FM9", "ELDIEGO", "MDJ", "SAFA"};

/*
 * El proceso pasa a trabajar como servidor concurrente, lanzando hilos a los clientes que se conecten
 * atendiéndolos con la funcion accionHilo
 */
void ServidorConcurrente(char* ip, int puerto, Emisor nombre, t_list** listaDeHilos,
		bool* terminar, void (*accionHilo)(void* socketFD)) {
	char* n = Emisores[nombre];
	printf("Iniciando Servidor %s\n", n);
	*terminar = false;
	*listaDeHilos = list_create();
	int socketFD = StartServidor(ip, puerto);
	struct sockaddr_in their_addr; // información sobre la dirección del cliente
	int new_fd;
	socklen_t sin_size;

	while(!*terminar) { // Loop Principal
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(socketFD, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("\nNueva conexion de %s en " "socket %d\n", inet_ntoa(their_addr.sin_addr), new_fd);
		structHilo* itemNuevo = malloc(sizeof(structHilo));
		itemNuevo->socket = new_fd;
		pthread_create(&(itemNuevo->hilo), NULL, (void*)accionHilo, &new_fd);
		list_add(*listaDeHilos, itemNuevo);
	}
	printf("Apagando Servidor");
	close(socketFD);
	//libera los items de lista de hilos , destruye la lista y espera a que termine cada hilo.
	list_destroy_and_destroy_elements(*listaDeHilos, LAMBDA(void _(void* elem) {
			pthread_join(((structHilo*)elem)->hilo, NULL);
			free(elem); }));

}

int ConectarAServidor(int puertoAConectar, char* ipAConectar, Emisor servidor, Emisor cliente,
					  void RecibirElHandshake(int socketFD, Emisor emisor)) {
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in direccion;

	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(puertoAConectar);
	direccion.sin_addr.s_addr = inet_addr(ipAConectar);
	memset(&(direccion.sin_zero), '\0', 8);

	while (connect(socketFD, (struct sockaddr *) &direccion, sizeof(struct sockaddr))<0)
		sleep(1); //Espera un segundo y se vuelve a tratar de conectar.
	EnviarHandshake(socketFD, cliente);
	RecibirElHandshake(socketFD, servidor);
	return socketFD;

}

int ConectarAServidorCpu(int puertoAConectar, char* ipAConectar, Emisor servidor,
		Emisor cliente, void RecibirElHandshake(int socketFD, Emisor emisor), void EnviarElHandshake(int socketFD, Emisor emisor)) {
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in direccion;

	direccion.sin_family = AF_INET;
	direccion.sin_port = htons(puertoAConectar);
	direccion.sin_addr.s_addr = inet_addr(ipAConectar);
	memset(&(direccion.sin_zero), '\0', 8);

	while (connect(socketFD, (struct sockaddr *) &direccion, sizeof(struct sockaddr))<0)
		sleep(1); //Espera un segundo y se vuelve a tratar de conectar.
	EnviarElHandshake(socketFD, cliente);
	RecibirElHandshake(socketFD, servidor);
	return socketFD;

}

int StartServidor(char* MyIP, int MyPort) // obtener socket a la escucha
{
	struct sockaddr_in myaddr; // dirección del servidor
	int yes = 1; // para setsockopt() SO_REUSEADDR, más abajo
	int SocketEscucha;

	if ((SocketEscucha = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(SocketEscucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) // obviar el mensaje "address already in use" (la dirección ya se está usando)
	{
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(MyIP);
	myaddr.sin_port = htons(MyPort);
	memset(&(myaddr.sin_zero), '\0', 8);

	if (bind(SocketEscucha, (struct sockaddr*) &myaddr, sizeof(myaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	// escuchar
	if (listen(SocketEscucha, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	return SocketEscucha;
}

//envia la estructura paquete(header+payload) al socket establecido
bool EnviarPaquete(int socketCliente, Paquete* paquete) {
	int cantAEnviar = sizeof(Header) + paquete->header.tamPayload;
	void* datos = malloc(cantAEnviar);
	memmove(datos, &(paquete->header), TAMANIOHEADER);
	if (paquete->header.tamPayload > 0) //No sea handshake
		memmove(datos + TAMANIOHEADER, (paquete->Payload), paquete->header.tamPayload);

	//Paquete* punteroMsg = datos;
	int enviado = 0; //bytes enviados
	int totalEnviado = 0;
	bool valor_retorno=true;
	do {
		enviado = send(socketCliente, datos + totalEnviado, cantAEnviar - totalEnviado, 0);
		//largo -= totalEnviado;
		totalEnviado += enviado;
		if(enviado==-1){
			valor_retorno=false;
			break;
		}
		//punteroMsg += enviado; //avanza la cant de bytes que ya mando
	} while (totalEnviado != cantAEnviar);
	free(datos);
	return valor_retorno;
}

//como la funcion anterior pero especificando de quién y qué tipo de mensaje (establecido en enum)
bool EnviarDatosTipo(int socketFD, Emisor emisor, void* datos, int tamDatos, Tipo tipoMensaje){
	Paquete* paquete = malloc(sizeof(Paquete));
	//paquete->Payload=malloc(tamDatos);
	paquete->header.tipoMensaje = tipoMensaje;
	paquete->header.emisor = emisor;
	uint32_t r = 0;
	bool valor_retorno;
	if(tamDatos<=0 || datos==NULL){
		paquete->header.tamPayload = sizeof(uint32_t);
		paquete->Payload = &r;
	} else {
		paquete->header.tamPayload = tamDatos;
		//memmove(paquete->Payload,datos,tamDatos);
		paquete->Payload=datos;
	}
	valor_retorno=EnviarPaquete(socketFD, paquete);
	//free(paquete->Payload);
	free(paquete);
	return valor_retorno;
}

bool EnviarMensaje(int socketFD, char* msg, Emisor emisor) {
	Paquete paquete;
	paquete.header.emisor = emisor;
	paquete.header.tipoMensaje = ESSTRING;
	paquete.header.tamPayload = strlen(msg) + 1;
	paquete.Payload = msg;
	return EnviarPaquete(socketFD, &paquete);
}

void EnviarHandshake(int socketFD, Emisor emisor) {
	Paquete* paquete = malloc(TAMANIOHEADER);
	Header header;
	header.tipoMensaje = ESHANDSHAKE;
	header.tamPayload = 0;
	header.emisor = emisor;
	paquete->header = header;
	EnviarPaquete(socketFD, paquete);
	free(paquete);

}


bool EnviarDatos(int socketFD, Emisor emisor, void* datos, int tamDatos) {
	return EnviarDatosTipo(socketFD, emisor, datos, tamDatos, ESDATOS);
}

void RecibirHandshake(int socketFD, Emisor emisor) {
	Header header;
	int resul = RecibirDatos(&header, socketFD, TAMANIOHEADER);
	if (resul > 0) { // si no hubo error en la recepcion
		if (header.emisor == emisor) {
			if (header.tipoMensaje == ESHANDSHAKE)
				printf("\nConectado con el servidor\n");
			else
				perror("Error de Conexion, no se recibio un handshake\n");
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	}
}

int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir) {
	void* datos = malloc(cantARecibir);
	int recibido = 0;
	int totalRecibido = 0;

	do {
		recibido = recv(socketFD, datos + totalRecibido, cantARecibir - totalRecibido, 0);
		totalRecibido += recibido;
		log_debug(logger, "Recibi %d/%d", recibido, totalRecibido);
	} while (totalRecibido != cantARecibir && recibido > 0);
	memcpy(paquete, datos, cantARecibir);
	free(datos);
	if (recibido < 0) {
		printf("Cliente Desconectado\n");
		//TODO: Preguntar que socket se desconecto, si no es CPU o si es el ultimo CPU tiene que morir todo.
		close(socketFD); // ¡Hasta luego!
		//exit(1);
	} else if (recibido == 0) {
		printf("Fin de Conexion en socket %d\n", socketFD);
		close(socketFD); // ¡Hasta luego!
	}

	return recibido;
}

int RecibirPaqueteServidor(int socketFD, Emisor receptor, Paquete* paquete) {
	paquete->Payload = NULL;
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake
			printf("Se establecio conexion con\n");
			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
		} else if (paquete->header.tamPayload > 0){ //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = malloc(paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}
	return resul;
}

int RecibirPaqueteServidorSafa(int socketFD, Emisor receptor, Paquete* paquete) {
	paquete->Payload = NULL;
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tamPayload > 0) {
		paquete->Payload = malloc(paquete->header.tamPayload);
		resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
	}
	return resul;
}

int RecibirPaqueteCliente(int socketFD, Paquete* paquete) {
	paquete->Payload = NULL;
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tamPayload > 0) { //si no hubo error ni es un handshake
		paquete->Payload = malloc(paquete->header.tamPayload);
		resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
	}
	return resul;
}

int recibir_paquete(int socket, Paquete *paquete)
{
	recibir_partes(socket, &paquete->header, TAMANIOHEADER);
	log_debug(logger, "Recibi el header");

	if (paquete->header.tamPayload == 0)
		return TAMANIOHEADER;

	paquete->Payload = malloc(paquete->header.tamPayload);
	log_debug(logger, "Recibo el payload de tamanio %i", paquete->header.tamPayload);
	recibir_partes(socket, paquete->Payload, paquete->header.tamPayload);

	return TAMANIOHEADER + paquete->header.tamPayload;
}

void recibir_partes(int socket, void *buffer, int cant_a_recibir)
{
	int recibido = 0;
	int total_recibido = 0;
	int len = transfer_size;

	while (total_recibido != cant_a_recibir)
	{
		if (cant_a_recibir < transfer_size)
			len = cant_a_recibir;

		recibido = recv(socket, buffer + total_recibido, len, 0);
		log_debug(logger, "%i/%i", recibido, cant_a_recibir);

		//man recv en el caso de -1 es error pero tambien lo matamos
		if (recibido <= 0)
			_exit_with_error(socket, "No pudo recibir el paquete", NULL);

		total_recibido += recibido;
	}

}

Header cargar_header(int tamanio_payload, Tipo tipo_mensaje, Emisor emisor)
{   
	Header header;
	header.tamPayload = tamanio_payload;
	header.tipoMensaje = tipo_mensaje;
	header.emisor = emisor;
	return header;
}
