#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"

context(test_plp) {

	describe("Planificador largo plazo") {

		DTB *dtb1;
		DTB *dtb2;
		char *path_escriptorio1 = "test1";
		char *path_escriptorio2 = "test2";
		int numero_pid = 1;
		int *p;
		t_list *lista_plp;
		t_list *lista_nuevos;

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

		DTB* crearDTB(char* path) {

			DTB* DTBNuevo = malloc (sizeof(DTB));
			DTBNuevo->PC = 1;
			DTBNuevo->estado = ESTADO_NUEVO;
			DTBNuevo->flagInicializacion = 1;
			DTBNuevo->pathEscriptorio = string_duplicate(path); //pathEscriptorio es un char*
			DTBNuevo->gdtPID = numero_pid;
			numero_pid++;

			return DTBNuevo;
		}
		
		bool coincide_pid(int* pid, void* DTB_comparar) {
			return ((DTB*)DTB_comparar)->gdtPID == *pid;
		}

		DTB* devuelve_DTB_asociado_a_pid_de_lista(t_list* lista, int* pid) {
			bool compara_con_DTB(void* DTB) {
				return coincide_pid(pid, DTB);
			}
			return list_remove_by_condition(lista, compara_con_DTB);
		}

		void notificar_al_plp(t_list* lista, int* pid) {
			DTB* DTB_a_mover = devuelve_DTB_asociado_a_pid_de_lista(lista, pid);
			list_add(lista_plp, DTB_a_mover);
		}

		void liberar_dtb() {
			free(dtb1->pathEscriptorio);
			free(dtb1);
			free(dtb2->pathEscriptorio);
			free(dtb2);
		}

		void dtb_mock() {
			dtb1 = crearDTB(path_escriptorio1);
			dtb2 = crearDTB(path_escriptorio2);
		}


		before {
			dtb_mock();
			lista_plp = list_create();
			lista_nuevos = list_create();
			numero_pid = 1;
			list_add(lista_nuevos, dtb1);
			list_add(lista_nuevos, dtb2);
		} end

		after {
			liberar_dtb();
			list_destroy(lista_plp);
		} end

		it("Logra crear un dtb al path indicado") {
			should_string(dtb1->pathEscriptorio) be equal to(path_escriptorio1);
		} end

		it("Logra crear DTBs con pid 1 y 2") {
			should_int(dtb1->gdtPID) be equal to (1);
			should_int(dtb2->gdtPID) be equal to (2);
		} end

		it("Logra notificar al plp que se cargo pid 1") {
			should_int(list_size(lista_plp)) be equal to(0);
			should_int(list_size(lista_nuevos)) be equal to(2);
			
			int i = 1;
			p = &i;
			notificar_al_plp(lista_nuevos, p);
			should_int(list_size(lista_plp)) be equal to(1);

			DTB *dtb = list_get(lista_plp, 0);
			should_int(dtb->gdtPID) be equal to(1);
		} end

	} end

}
