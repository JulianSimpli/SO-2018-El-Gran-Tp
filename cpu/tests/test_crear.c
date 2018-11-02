#include <cspecs/cspec.h>
#include "../../Bibliotecas/sockets.h"

context(test_primitives) {

	describe("CPU puede ejecutar crear") {

		char *message = "crear /equipos/Racing.txt 11";
		/*
		La primitiva crear permitirá crear un nuevo archivo en el MDJ. Para esto, se deberá utilizar de intermediario a El Diego.
		Sintaxis
		Recibe como parámetro el path y la cantidad de líneas que tendrá el archivo.
		Funcionalidad
		Se ejecutarán los siguientes pasos:
		Se deberá enviar a El Diego el archivo a crear con la cantidad de líneas necesarias. El CPU desalojará el programa G.DT y S-AFA lo bloqueará.
		El Diego enviará los datos a MDJ para que este creé el nuevo archivo.
		MDJ leerá dichos datos, verificará si tiene espacio necesario, por medio de sus estructuras administrativas, y creará el archivo. El archivo creado debe contener internamente la cantidad de líneas indicadas con cada línea vacía (con “\n”)
		MDJ informará a El Diego la finalización de dicha operación y este último se lo comunicará a S-AFA para que desbloquee el programa G.DT.
		En caso de que suceda algún error en dicha operatoria se debe abortar el programa G.DT
		Errores Esperables
		Los errores esperables son:
		50001: Archivo ya existente.
		50002: Espacio insuficiente.
		*/

		it("Puede ejecutar bien") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 50001") {
			should_bool(false) be truthy;
		} end

		it("Puede terminar por error 50002") {
			should_bool(false) be truthy;
		} end

	} end

}
