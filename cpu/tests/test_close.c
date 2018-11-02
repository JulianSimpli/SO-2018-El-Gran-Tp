#include <cspecs/cspec.h>
#include "../../Bibliotecas/sockets.h"

context(test_primitives) {

	describe("CPU puede ejecutar close") {

		char *message = "close /equipos/Racing.txt";

		/*
		Funcionalidad
		Se ejecutarán los siguientes pasos:
		Verificará que el archivo solicitado esté abierto por el G.DT. Para esto se utilizarán la estructura administrativa del DTB. En caso que no se encuentre, se abortará el G.DT.
		Se enviará a FM9 la solicitud para liberar la memoria del archivo deseado. Para esto, se le pasaran los parámetros necesarios para que FM9 pueda realizar la operación.
		Se liberará de las estructuras administrativas del DTB la referencia a dicho archivo.
		Errores Esperables
		Los errores esperados son:
		40001: El archivo no se encuentra abierto.
		40002: Fallo de segmento/memoria.
		*/

		it("Puede ejecutar bien") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 400001") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 400002") {
			should_bool(false) be truthy;
		} end


	} end

}
