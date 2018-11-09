#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <stdio.h>

context(test_plp)
{
	describe("Planificador largo plazo")
	{
		DTB *dtb1;
		char *path_escriptorio = "test1";
		int numero_pid = 1;
		int *p;
		t_list *lista_listos;
		t_list *lista_nuevos;
		
		char *string_duplicate(char *string) {
			size_t len = strlen(string);
			char *ret = malloc(len);
			strcpy(ret, string);
			return ret;
		}

		ArchivoAbierto *_DTB_crear_archivo(int cant_lineas, char *path)
		{
			ArchivoAbierto *archivo = malloc(sizeof(ArchivoAbierto));
			archivo->cantLineas = cant_lineas;
			archivo->path = malloc(strlen(path));
			strcpy(archivo->path, path);
			return archivo;
		}

		void DTB_agregar_archivo(DTB *dtb, int cant_lineas, char *path)
		{
			ArchivoAbierto *archivo = _DTB_crear_archivo(cant_lineas, path);
			list_add(dtb->archivosAbiertos, archivo);
		}

		ArchivoAbierto *DTB_obtener_escriptorio(DTB *dtb)
		{
			return list_get(dtb->archivosAbiertos, 0);
		}

		DTB *crear_dtb(int pid_asociado, char* path, int flag_inicializacion) {
			DTB* dtb_nuevo = malloc(sizeof(DTB));
			dtb_nuevo->flagInicializacion = flag_inicializacion;
			dtb_nuevo->PC = 1;
			switch(flag_inicializacion) {
				case 0: {
					dtb_nuevo->gdtPID = pid_asociado;
					break;
				}
				case 1: {
					dtb_nuevo->gdtPID = numero_pid;
					numero_pid++;
					break;
				}
				default: {
					printf("flag_inicializacion invalida");
					free(dtb_nuevo);
					return dtb_nuevo;
				}
			}

			dtb_nuevo->archivosAbiertos = list_create();
			DTB_agregar_archivo(dtb_nuevo, 0, path);

			return dtb_nuevo;
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
		void notificar_al_plp(int* pid) {
			DTB* DTB_a_mover = devuelve_DTB_asociado_a_pid_de_lista(lista_nuevos, pid);
			list_add(lista_listos, DTB_a_mover);
		}

		void liberar_archivo_abierto(void *archivo)
		{
			free(((ArchivoAbierto *)archivo)->path);
			free(archivo);
		}

		void liberar_dtb(void *dtb) {
			list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
			free(dtb);
		}

		void dtb_mock() {
			dtb1 = crear_dtb(0, path_escriptorio, 1);
		}

		before {
			dtb_mock();
			lista_listos = list_create();
			lista_nuevos = list_create();
			numero_pid = 1;
			list_add(lista_nuevos, dtb1);
		} end

		after {
			liberar_dtb(dtb1);
			list_destroy(lista_listos);
			list_destroy(lista_nuevos);
		} end

		it("Logra crear DTBs con pid 1 y 2") {
			should_int(dtb1->gdtPID) be equal to (1);
		} end

		it("Logra notificar al plp que se cargo ") {
			should_int(list_size(lista_listos)) be equal to(0);
			should_int(list_size(lista_nuevos)) be equal to(1);
			
			int i = 1;
			p = &i;
			notificar_al_plp(p);
			should_int(list_size(lista_listos)) be equal to(1);

			DTB *dtb = list_get(lista_listos, 0);
			should_int(dtb->gdtPID) be equal to(1);
		} end

	} end
}
