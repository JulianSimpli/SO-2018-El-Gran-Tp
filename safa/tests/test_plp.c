#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <stdio.h>

typedef enum {
	DTB_NUEVO, DTB_LISTO, DTB_EJECUTANDO, DTB_BLOQUEADO, DTB_FINALIZADO,
	CPU_LIBRE, CPU_OCUPADA
} Estado;

typedef struct DTB_info {
	u_int32_t gdtPID;
	Estado estado;
	time_t tiempo_respuesta;
} DTB_info;

context(test_plp)
{
	describe("Planificador largo plazo")
	{
		DTB *dtb1;
		char *path_escriptorio;
		int numero_pid;
		int MULTIPROGRAMACION;
		int procesos_en_memoria;
		t_list *lista_listos;
		t_list *lista_nuevos;
		t_list *lista_info_dtb;
		int *p;
		int i;

		
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

		DTB *crear_dtb(int pid, char* path, int flag_inicializacion) {
			DTB* dtb_nuevo = malloc(sizeof(DTB));
			dtb_nuevo->flagInicializacion = flag_inicializacion;
			dtb_nuevo->PC = 0;
			switch(flag_inicializacion) {
				case 0:
				{
					dtb_nuevo->gdtPID = pid;
					break;
				}
				case 1:
				{
					dtb_nuevo->gdtPID = numero_pid;
					numero_pid++;

					DTB_info *info_dtb = malloc(sizeof(DTB_info));
					info_dtb->gdtPID = dtb_nuevo->gdtPID;
					info_dtb->estado = DTB_NUEVO;
					list_add(lista_info_dtb, info_dtb);
					break;
				}
				default:
				{
					printf("flag_inicializacion invalida");
				}
			}

			dtb_nuevo->archivosAbiertos = list_create();
			DTB_agregar_archivo(dtb_nuevo, 0, path);

			return dtb_nuevo;
		}

		bool coincide_pid(int pid, void* DTB_comparar)
		{
			return ((DTB *)DTB_comparar)->gdtPID == pid;
		}

		bool coincide_pid_info(int pid, void *info_dtb)
		{
			return ((DTB_info *)info_dtb)->gdtPID == pid;
		}

		DTB_info *info_asociada_a_pid(t_list *lista, int pid)
		{
			bool compara_con_info(void *info_dtb)
			{
				return coincide_pid_info(pid, info_dtb);
			}
			return list_find(lista, compara_con_info);
		}

		DTB* DTB_asociado_a_pid(t_list* lista, int pid)
		{
			bool compara_con_dtb(void* DTB) {
				return coincide_pid(pid, DTB);
			}
			return list_remove_by_condition(lista, compara_con_dtb);
		}

		void notificar_al_plp(int* pid) {
			DTB* DTB_a_mover = DTB_asociado_a_pid(lista_nuevos, *pid);
			list_add(lista_listos, DTB_a_mover);
			procesos_en_memoria++;
		}

		bool permite_multiprogramacion() {
			return procesos_en_memoria < MULTIPROGRAMACION;
		}

		void desbloquear_dtb_dummy(DTB* dtb_nuevo)
		{
			DTB* dtb_dummy = malloc (sizeof(DTB));
			ArchivoAbierto* escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
			dtb_dummy = crear_dtb(dtb_nuevo->gdtPID, escriptorio->path, 0);
			list_add(lista_listos, dtb_dummy);
		}

		void planificador_largo_plazo()
		{
			while(1)
			{
				if(permite_multiprogramacion() && !list_is_empty(lista_nuevos))
				{
					DTB* dtb_nuevo = list_get(lista_nuevos, 0);
					desbloquear_dtb_dummy(dtb_nuevo);
					break; // Para testear
				}
			}
		}

		void liberar_archivo_abierto(void *archivo)
		{
			free(((ArchivoAbierto *)archivo)->path);
			free(archivo);
		}

		void liberar_info(void *dtb)
		{
			int pid = ((DTB *)dtb)->gdtPID;
			bool coincide_info(void *info_dtb)
			{
				return coincide_pid_info(pid, info_dtb);
			}
			list_remove_and_destroy_by_condition(lista_info_dtb, coincide_info, free);
		}

		void liberar_dtb(void *dtb)
		{
			if (((DTB *)dtb)->flagInicializacion == 1) liberar_info(dtb);
			
			list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
			free(dtb);
		}

		void bloquear_dummy(int *pid)
		{
			bool compara_dummy(void *dtb)
			{
				return coincide_pid(*pid, dtb);
			}
			list_remove_and_destroy_by_condition(lista_listos, compara_dummy, liberar_dtb);
		}

		void dtb_mock() {
			dtb1 = crear_dtb(0, path_escriptorio, 1);
		}
		void inicializar_variables_y_listas()
		{
			lista_nuevos = list_create();
			lista_listos = list_create();
			lista_info_dtb = list_create();
			path_escriptorio = string_duplicate("path1");
			numero_pid = 1;
			MULTIPROGRAMACION = 3;
			procesos_en_memoria = 0;
			i = numero_pid;
			p = &i;
		}

		before
		{
			inicializar_variables_y_listas();
			dtb_mock();
			list_add(lista_nuevos, dtb1);
		}
		end
		after
		{
			liberar_dtb(dtb1);
			list_destroy(lista_info_dtb);
			list_destroy(lista_listos);
			list_destroy(lista_nuevos);
		}
		end

		it("Logra crear DTB mock con atributos e info correcta") {
			should_int(dtb1->gdtPID) be equal to (1);
			should_int(dtb1->PC) be equal to (0);
			should_int(dtb1->flagInicializacion) be equal to (1);

            ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb1);
            should_string(escriptorio->path) be equal to ("path1");
            should_int(escriptorio->cantLineas) be equal to (0);

			DTB_info * info_dtb = list_get(lista_info_dtb,0);
			should_int(info_dtb->gdtPID) be equal to (1);
			should_int(info_dtb->estado) be equal to (DTB_NUEVO);
		} end

		it("Notificar al plp agrega dtb a lista listos")
		{			
			should_int(list_size(lista_listos)) be equal to (0);
			notificar_al_plp(p);
			should_int(list_size(lista_listos)) be equal to (1);

			DTB *dtb = list_get(lista_listos, 0);
			should_int(dtb->gdtPID) be equal to (1);
		}
		end

		it("Planificador largo plazo agrega dummmy a listos")
		{
			should_int(list_size(lista_listos)) be equal to (0);
			planificador_largo_plazo();
			should_int(list_size(lista_listos)) be equal to (1);

			should_int(list_size(lista_info_dtb)) be equal to (1);
		}
		end

		it("Planificador largo plazo crea dummy con atributos correctos")
		{
			planificador_largo_plazo();
			DTB *dtb_dummy = list_get(lista_listos, 0);
			should_int(dtb_dummy->gdtPID) be equal to (1);
            should_int(dtb_dummy->PC) be equal to (0);
            should_int(dtb_dummy->flagInicializacion) be equal to (0);
            
            ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb_dummy);
            should_string(escriptorio->path) be equal to ("path1");
            should_int(escriptorio->cantLineas) be equal to (0);
		}
		end

		it("Bloquear dummy saca al dummy de ready. Notificar al plp agrega al dtb")
		{
			planificador_largo_plazo();
			DTB *dtb_dummy = list_get(lista_listos, 0);
			should_int(dtb_dummy->flagInicializacion) be equal to (0);

			bloquear_dummy(p);
			should_int(list_size(lista_listos)) be equal to (0);

			notificar_al_plp(p);
			DTB *dtb = list_get(lista_listos, 0);
			should_int(dtb->flagInicializacion) be equal to (1);
			should_int(list_size(lista_listos)) be equal to (1);

		}
		end
	}
	end
}
