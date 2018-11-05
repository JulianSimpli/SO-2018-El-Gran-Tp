#include <cspecs/cspec.h>
#include "../Bibliotecas/dtb.h"

context(test_dtb) {

	describe("serializa dtb") {

		DTB *un_dtb;
		ArchivoAbierto *un_archivo;
		ArchivoAbierto *otro_archivo;
		t_list *una_lista;
		char *path_archivo;
		int cant_lineas;

		before {
			un_archivo = malloc(sizeof(ArchivoAbierto));
			un_archivo->cantLineas = 10;
			char *ejemplo = "ejemplo.txt";
			un_archivo->path = malloc(strlen(ejemplo) + 1);
			memcpy(un_archivo->path, ejemplo, strlen(ejemplo));

			otro_archivo = malloc(sizeof(ArchivoAbierto));
			otro_archivo->cantLineas = 11;
			char *otro_ejemplo = "otro_ejemplo.txt";
			otro_archivo->path = malloc(strlen(otro_ejemplo) + 1);
			memcpy(otro_archivo->path, otro_ejemplo, strlen(otro_ejemplo));

			una_lista = list_create();
			list_add(una_lista, un_archivo);
			list_add(una_lista, otro_archivo);

			un_dtb = malloc(sizeof(DTB));
			char *path = "path";
			un_dtb->pathEscriptorio = malloc(strlen(path));
			memcpy(un_dtb->pathEscriptorio, path, strlen(path));

			un_dtb->archivosAbiertos = una_lista;

			path_archivo = "test";
			cant_lineas = 4;

		} end

		after {
			liberar_archivo_abierto(un_archivo);
			liberar_archivo_abierto(otro_archivo);
			list_destroy(una_lista);
			free(un_dtb->pathEscriptorio);
			free(un_dtb);
		} end

		it("Puede crear un archivo") {
			ArchivoAbierto *archivo = _DTB_crear_archivo(cant_lineas, path_archivo); 

			should_int(archivo->cantLineas) be equal to(4);
			should_string(archivo->path) be equal to("test");
		} end

		it("Puede liberar la memoria de un archivo") {
			ArchivoAbierto *archivo = _DTB_crear_archivo(cant_lineas, path_archivo); 
			liberar_archivo_abierto(archivo);
			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(2);
		} end

		it("Puede agregar un archivo a la lista de dtb") {
			_DTB_agregar_archivo(un_dtb, cant_lineas, path_archivo);
			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(3);
		} end

		it("Puede encontrar el escriptorio") {
			DTB *un_dtb = malloc(sizeof(DTB));
			char *path = "path";
			un_dtb->pathEscriptorio = malloc(strlen(path));
			memcpy(un_dtb->pathEscriptorio, path, strlen(path));

			un_dtb->archivosAbiertos = list_create();

			ArchivoAbierto *archivo = _DTB_crear_archivo(12, "escriptorio"); 
			list_add(un_dtb->archivosAbiertos, archivo);
			ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(un_dtb);
			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(1);
			should_string(escriptorio->path) be equal to("escriptorio");
		} end

		it("Puede remover un archivo de la lista") {

			_DTB_agregar_archivo(un_dtb, cant_lineas, path_archivo);
			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(3);
			
			//Tiene que buscar el archivo comparando el nombre
			//eso lo tengo en cpu
			//se parece pero no es
			//ArchivoAbierto *removed = _DTB_encontrar_archivo();
			//despues la llamo con el destoy tambien pero que sea otro test

			_DTB_remover_archivo(un_dtb, path_archivo);

			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(2);
		} end

		it("Puede remover un archivo de la lista y liberar memoria") {

		    ArchivoAbierto *archivo = _DTB_crear_archivo(cant_lineas, path_archivo);
    		list_add(un_dtb->archivosAbiertos, archivo);

			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(3);

			_DTB_remover_archivo(un_dtb, path_archivo);
			should_int(list_size(un_dtb->archivosAbiertos)) be equal to(2);
			//Hay que testear que quede en null el puntero liberado
			should_bool(false) be truthy;
		} end

		it("Puede liberar una lista del dtb") {
			should_bool(false) be truthy;
		} end

		it("Puede liberar un dtb") {
			should_bool(false) be truthy;
		} end

	} end

}