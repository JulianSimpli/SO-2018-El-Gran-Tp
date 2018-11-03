#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"

context(test_plp) {

	describe("Planificador largo plazo") {

		DTB *dtb;
		char *path_escriptorio = "test";
		int numeroPID = 1;
		t_list *lista_plp;

typedef enum {
	ESTADO_NUEVO,
	ESTADO_LISTO,
	ESTADO_EJECUTANDO,
	ESTADO_BLOQUEADO,
	ESTADO_FINALIZADO
} Estados;

char *string_duplicate(char *string) {
	size_t len = strlen(string);
	char *ret = malloc(len);
	strcpy(ret, string);
	return ret;
}

//Funciones asociadas a DTB
DTB* crearDTB(char* path) {

	DTB* DTBNuevo = malloc (sizeof(DTB));
	DTBNuevo->PC = 1;
	DTBNuevo->estado = ESTADO_NUEVO;
	DTBNuevo->flagInicializacion = 1;
	DTBNuevo->pathEscriptorio = string_duplicate(path); //pathEscriptorio es un char*
	DTBNuevo->gdtPID = numeroPID;
	numeroPID++;

	return DTBNuevo;
}

void notificar_al_PLP(int i) {
	list_add(lista_plp, dtb);	
}

		void liberar_dtb() {
			free(dtb->pathEscriptorio);
			free(dtb);
		}

		void dtb_mock() {
			dtb = crearDTB(path_escriptorio);
		}


		before {
			dtb_mock();
			lista_plp = list_create();
			numeroPID = 1;
		} end

		after {
			liberar_dtb();
			list_destroy(lista_plp);
		} end

		it("Logra crear un dtb al path indicado") {
			should_string(dtb->pathEscriptorio) be equal to(path_escriptorio);
		} end

		it("Logra encontrar segun numero de pid en la lista al dtb") {
			should_bool(false) be truthy;
		} end

		it("Logra agregar un dtb a la lista de plp") {
			should_int(list_size(lista_plp)) be equal to(0);
			notificar_al_PLP(1);
			should_int(list_size(lista_plp)) be equal to(1);
			DTB *dtb = list_get(lista_plp, 0);
			should_int(dtb->gdtPID) be equal to(1);
		} end

	} end

}
