#include <cspecs/cspec.h>
#include "../../Bibliotecas/sockets.h"

context(test_primitives) {

	describe("CPU puede ejecutar flush") {

		char *message = "flush /equipos/Racing.txt";

		/*
		La primitiva Flush permite guardar el contenido de FM9 a MDJ.
		Sintaxis
		Recibe como parámetro el path del archivo a guardar en MDJ.
			Ej: flush /equipos/Racing.txt
		Funcionalidad
		Se ejecutarán los siguientes pasos:
		Verificará que el archivo solicitado esté abierto por el G.DT. Para esto se utilizarán la estructura administrativa del DTB. En caso que no se encuentre, se abortará el G.DT.
		Se enviará una solicitud a El Diego indicando que se requiere hacer un Flush del archivo, enviando los parámetros necesarios para que pueda obtenerlo desde FM9 y guardarlo en MDJ.
		Se comunicará al proceso S-AFA que el G.DT se encuentra a la espera de una respuesta por parte de El Diego y S-AFA lo bloqueará.
		Errores Esperables
		Los errores esperados son:
		30001: El archivo no se encuentra abierto.
		30002: Fallo de segmento/memoria.
		30003: Espacio insuficiente en MDJ.
		30004: El archivo no existe en MDJ (fue borrado previamente).
		*/

		it("Puede ejecutar bien") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 300001") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 300002") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 300003") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 300004") {
			should_bool(false) be truthy;
		} end

	} end

}
