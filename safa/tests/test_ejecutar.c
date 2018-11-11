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
	// time_t tiempo_respuesta;
} DTB_info;

context(test_ejecutar)
{
    describe("primitiva consola ejecutar")
    {
        char *path_escriptorio1;
		char *path_escriptorio2;
		int numero_pid = 1;
		t_list *lista_listos;
		t_list *lista_nuevos;
		t_list *lista_info_dtb;


        char *string_duplicate(char *string)
        {
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
				case 0: {
					dtb_nuevo->gdtPID = pid;
					break;
				}
				case 1: {
					dtb_nuevo->gdtPID = numero_pid;
					numero_pid++;

					DTB_info *info_dtb = malloc(sizeof(DTB_info));
					info_dtb->gdtPID = dtb_nuevo->gdtPID;
					info_dtb->estado = DTB_NUEVO;
					list_add(lista_info_dtb, info_dtb);
					break;
				}
				default: {
					printf("flag_inicializacion invalida");
				}
			}

			dtb_nuevo->archivosAbiertos = list_create();
			DTB_agregar_archivo(dtb_nuevo, 0, path);

			return dtb_nuevo;
		}

		void ejecutar(char* path)
		{
			DTB* dtb_nuevo = crear_dtb(0, path, 1);
			list_add(lista_nuevos, dtb_nuevo);
		}

		bool coincide_pid(int pid, void* DTB_comparar)
		{
			return ((DTB *)DTB_comparar)->gdtPID == pid;
		}

		DTB* DTB_asociado_a_pid(t_list* lista, int pid)
		{
			bool compara_con_dtb(void* DTB) {
				return coincide_pid(pid, DTB);
			}
			return list_remove_by_condition(lista, compara_con_dtb);
		}

		bool coincide_pid_info(int pid, void *info_dtb)
		{
			return ((DTB_info *)info_dtb)->gdtPID == pid;
		}

		DTB_info *info_asociada_a_dtb(t_list *lista, DTB* dtb)
		{
			bool compara_con_info(void *info_dtb)
			{
				return coincide_pid_info(dtb->gdtPID, info_dtb);
			}
			return list_find(lista, compara_con_info);
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

        before
        {
            lista_nuevos = list_create();
            lista_listos = list_create();
			lista_info_dtb = list_create();
			path_escriptorio1 = "path1";
			path_escriptorio2 = "path2";
        } end

        after 
        {
            list_destroy_and_destroy_elements(lista_nuevos, liberar_dtb);
            list_destroy_and_destroy_elements(lista_listos, liberar_dtb);
			list_destroy(lista_info_dtb);
            numero_pid = 1;
        } end

        it("ejecutar path agrega dtb a lista nuevos")
        {
            should_int(list_size(lista_nuevos)) be equal to (0);
			ejecutar(path_escriptorio1);
			should_int(list_size(lista_nuevos)) be equal to (1);
        }
        end

		it("ejecutar path crea un dtb con los campos correctos")
        {
            ejecutar(path_escriptorio1);

			DTB *dtb1 = list_get(lista_nuevos, 0);
			should_int(dtb1->gdtPID) be equal to (1);
            should_int(dtb1->PC) be equal to (0);
            should_int(dtb1->flagInicializacion) be equal to (1);

            ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb1);
            should_string(escriptorio->path) be equal to ("path1");
            should_int(escriptorio->cantLineas) be equal to (0);
		}
        end

		it("ejecutar path agrega dtb_info a lista_info_dtb") {
            should_int(list_size(lista_info_dtb)) be equal to (0);
			ejecutar(path_escriptorio1);
            should_int(list_size(lista_info_dtb)) be equal to (1);
		}
        end

        it("ejecutar path crea dtb_info con los campos correctos")
        {
            ejecutar(path_escriptorio1);

            DTB_info *info_dtb = list_get(lista_info_dtb, 0);
			should_int(info_dtb->gdtPID) be equal to (1);
			should_int(info_dtb->estado) be equal to (DTB_NUEVO);
        }
        end

		it("Puede encontrar dtb_info a partir de pid")
		{
			ejecutar(path_escriptorio1);
			DTB *dtb = DTB_asociado_a_pid(lista_nuevos, 1);

			DTB_info *info_dtb = info_asociada_a_dtb(lista_info_dtb, dtb);

			should_int(info_dtb->gdtPID) be equal to (1);
			should_int(info_dtb->estado) be equal to (DTB_NUEVO);
		}
		end
    }
    end
}