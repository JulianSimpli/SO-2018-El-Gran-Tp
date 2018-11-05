#include <cspecs/cspec.h>
#include "../Bibliotecas/dtb.h"

context(test_dtb) {

	describe("deserializa dtb") {

		u_int32_t cant_archivos;
		u_int32_t cant_lineas;
		u_int32_t tamanio_direccion;
		char *direccion_archivo = "/equipos/River.txt";
		char *path_escriptorio = "/test.sh";
		DTB *dtb_mock;
		void *payload;
		void *lista_archivos_mock;
		ArchivoAbierto *archivo_abierto;

		void create_mock()
		{
			dtb_mock = malloc(sizeof(DTB));
			dtb_mock->gdtPID = 1;
			dtb_mock->PC = 2;
			dtb_mock->flagInicializacion = 0;
			dtb_mock->archivosAbiertos = list_create();
		}

		void mock_payload()
		{
			int tamanio = 0;
			DTB_agregar_archivo(dtb_mock, 10, path_escriptorio);
			payload = DTB_serializar(dtb_mock, &tamanio);
		}

		void mock_archivo_abierto()
		{
			cant_lineas = 11;
			tamanio_direccion = strlen(direccion_archivo);
			size_t tamanio = sizeof(u_int32_t);
			archivo_abierto = malloc(tamanio * 2 + tamanio_direccion);
		}

		void mock_lista_archivos()
		{
			cant_archivos = 1;

			//Por ahora es un solo ArchivoAbierto
			//Primero agrega un int que indica cuantos hay en la lista
			//Depues el primer int es cantidad de lineas el segundo el len
			//Y por ultimo el tamanio del string
			size_t tamanio = sizeof(u_int32_t);
			lista_archivos_mock = malloc(tamanio * 2 + tamanio_direccion);
			int offset = 0;
			memcpy(lista_archivos_mock, &cant_archivos, tamanio);
			offset += tamanio;
			memcpy(lista_archivos_mock + offset, &cant_lineas, tamanio);
			offset += tamanio;
            memcpy(lista_archivos_mock + offset, &tamanio_direccion, tamanio);
			offset += tamanio;
            memcpy(lista_archivos_mock + offset, direccion_archivo, tamanio_direccion);
		}

		void free_payload_mock()
		{
			free(payload);
		}

		void free_dtb_mock()
		{
			free(dtb_mock);
		}

		void free_lista_archivos_mock()
		{
			free(lista_archivos_mock);
		}

		before {
			mock_archivo_abierto();
			mock_lista_archivos();
			create_mock();
			mock_payload();
		} end

		after {
			free_payload_mock();
			free_dtb_mock();
			free_lista_archivos_mock();
		} end

		/**
		 * Nos sirve para poder mantener consistente el peso del dtb.
		 * Solo va a fallar si cambia la definicion de los atributos staticos del dtb
		 */
		it("Puede crear un dtb") {
			size_t tamanio = sizeof(DTB);
			should_int(tamanio) be equal to(24);
			should_int(dtb_mock->gdtPID) be equal to(1);
		} end

		it("Puede copiar datos staticos que vienen en payload") {
			DTB *dtb = malloc(sizeof(DTB));
			int offset = 0;
			//Puedo consumir los 5 primeros int de una vez 
			DTB_cargar_estaticos(dtb, payload, &offset);
			should_int(dtb->gdtPID) be equal to(1);
			should_int(dtb->PC) be equal to(2);
			should_int(dtb->flagInicializacion) be equal to(0);
		} end

		it("Puede copiar datos staticos y deja bien posicionado el offset") {
			DTB *dtb = malloc(sizeof(DTB));
			int offset = 0;
			DTB_cargar_estaticos(dtb, payload, &offset);
			should_int(offset) be equal to(12);
		} end

		it("Puede copiar el path del escriptorio") {
			DTB *dtb = DTB_deserializar(payload);
			ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
			should_string(escriptorio->path) be equal to(path_escriptorio);
		} end

		it("Puede crear lista a partir de payload") {
			DTB *dtb = malloc(sizeof(DTB));
			dtb->archivosAbiertos = list_create();

			int desplazamiento = 0;
			DTB_cargar_archivos_abiertos(dtb->archivosAbiertos, lista_archivos_mock, &desplazamiento);
			should_int(list_size(dtb->archivosAbiertos)) be equal to(cant_archivos);

			ArchivoAbierto *archivo = list_get(dtb->archivosAbiertos, 0);
			should_int(archivo->cantLineas) be equal to(cant_lineas);
			should_string(archivo->path) be equal to(direccion_archivo);
		} end

		it("Puede deserializar dtb") {
			DTB *dtb = DTB_deserializar(payload);
			ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb);
			should_int(escriptorio->cantLineas) be equal to(10);
			should_string(escriptorio->path) be equal to(path_escriptorio);
			/*
			should_int(list_size(dtb->archivosAbiertos)) be equal to(cant_archivos);
			ArchivoAbierto *archivo = list_get(dtb->archivosAbiertos, 0);
			should_int(archivo->cantLineas) be equal to(cant_lineas);
			should_string(archivo->path) be equal to(direccion_archivo);
			*/
		} end


	} end

}