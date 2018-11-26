#include <cspecs/cspec.h>
#include "../Bibliotecas/dtb.h"

context(test_dtb) {

	describe("serializa dtb") {

		DTB *un_dtb;
		ArchivoAbierto *un_archivo;
		ArchivoAbierto *otro_archivo;
		t_list *una_lista;

		before {
			un_archivo = malloc(sizeof(ArchivoAbierto));
			un_archivo->cantLineas = 10;
			char *ejemplo = "ejemplo.txt";
			un_archivo->path = malloc(strlen(ejemplo) + 1);
			memcpy(un_archivo->path, ejemplo, strlen(ejemplo) + 1);

			otro_archivo = malloc(sizeof(ArchivoAbierto));
			otro_archivo->cantLineas = 11;
			char *otro_ejemplo = "otro_ejemplo.txt";
			otro_archivo->path = malloc(strlen(otro_ejemplo) + 1);
			memcpy(otro_archivo->path, otro_ejemplo, strlen(otro_ejemplo) + 1);

			una_lista = list_create();
			list_add(una_lista, un_archivo);
			list_add(una_lista, otro_archivo);

			un_dtb = malloc(sizeof(DTB));
			un_dtb->gdtPID = 1;
			un_dtb->PC = 10;
			un_dtb->flagInicializacion = 1;
			un_dtb->entrada_salidas = 5;

			un_dtb->archivosAbiertos = una_lista;
		} end

		after {
			free(un_archivo);
			free(otro_archivo);
			list_destroy(una_lista);
			free(un_dtb);
		} end

		it("Puede serializar y deserializar 4 ints") {
			int tamanio_serializado = 0;
			void *serializado = DTB_serializar_estaticos(un_dtb, &tamanio_serializado);
			should_int(tamanio_serializado) be equal to (16);

			int offset = 0;
			DTB *dtb = malloc(sizeof(DTB));
			DTB_cargar_estaticos(dtb, serializado, &offset);
			should_int(offset) be equal to (16);
			should_int(dtb->gdtPID) be equal to (1);
			should_int(dtb->PC) be equal to (10);
			should_int(dtb->flagInicializacion) be equal to (1);
			should_int(dtb->entrada_salidas) be equal to (5);
			free(dtb);
		} end

		it("Puede serializar y deserializar archivo") {
			int tamanio_serializado = 0;
			void *serializado = DTB_serializar_archivo(un_archivo, &tamanio_serializado);
			should_int(tamanio_serializado) be equal to(19);
			
			int desplazamiento = 0;
        	ArchivoAbierto *deserializado = DTB_leer_struct_archivo(serializado, &desplazamiento);
			should_int(deserializado->cantLineas) be equal to(10);
			should_string(deserializado->path) be equal to("ejemplo.txt");

			int tamanio_2 = 0;
			void *serializado_2 = DTB_serializar_archivo(otro_archivo, &tamanio_2);
			should_int(tamanio_2) be equal to(24);
			
			int desplazamiento_2 = 0;
        	ArchivoAbierto *deserializado_2 = DTB_leer_struct_archivo(serializado_2, &desplazamiento_2);
			should_int(deserializado_2->cantLineas) be equal to(11);
			should_string(deserializado_2->path) be equal to("otro_ejemplo.txt");
		} end

		it("Puede serializar y deserializar lista") {
			int tamanio_lista_serializada = 0;
			void *lista_serializada = DTB_serializar_lista(una_lista, &tamanio_lista_serializada);
			should_int(tamanio_lista_serializada) be equal to(43);

			void *data = malloc(tamanio_lista_serializada + sizeof(u_int32_t));
			int cant = 2;
			memcpy(data, &cant, sizeof(u_int32_t));
			memcpy(data + sizeof(u_int32_t), lista_serializada, tamanio_lista_serializada);
			t_list *lista = list_create(); 
			int desplazamiento = 0;
			DTB_cargar_archivos_abiertos(lista, data, &desplazamiento);
			should_int(list_size(lista)) be equal to(2);

			ArchivoAbierto *a = list_get(lista, 0);
			should_int(a->cantLineas) be equal to(10);
			should_string(a->path) be equal to("ejemplo.txt");

			ArchivoAbierto *b = list_get(lista, 1);
			should_int(b->cantLineas) be equal to(11);
			should_string(b->path) be equal to("otro_ejemplo.txt");
		} end

		it("Puede serializar y deserializar dtb entero") {
			int tamanio_dtb = 0;
			void *dtb_serializado = DTB_serializar(un_dtb, &tamanio_dtb);
			should_int(tamanio_dtb) be equal to(63);

			DTB *dtb = DTB_deserializar(dtb_serializado);
			
			ArchivoAbierto *a = list_get(dtb->archivosAbiertos, 0);
			should_int(a->cantLineas) be equal to(10);
			should_string(a->path) be equal to("ejemplo.txt");

			ArchivoAbierto *b = list_get(dtb->archivosAbiertos, 1);
			should_int(b->cantLineas) be equal to(11);
			should_string(b->path) be equal to("otro_ejemplo.txt");

		} end

	} end

}