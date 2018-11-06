#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <stdio.h>

context(test_ejecutar)
{
    describe("primitiva consola ejecutar")
    {
        char *path_escriptorio1 = "path1";
		char *path_escriptorio2 = "path2";
		int numero_pid = 1;
		t_list *lista_plp;
		t_list *lista_nuevos;

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

		DTB *crear_dtb(int pid_asociado, char* path, int flag_inicializacion)
        {
			DTB* dtb_nuevo = malloc(sizeof(DTB));
			dtb_nuevo->flagInicializacion = flag_inicializacion;
			dtb_nuevo->PC = 1;
			switch(flag_inicializacion) {
				case 0:
                {
					dtb_nuevo->gdtPID = pid_asociado;
					break;
				}
				case 1:
                {
					dtb_nuevo->gdtPID = numero_pid;
					numero_pid++;
					break;
				}
				default:
                {
					printf("flag_inicializacion invalida");
					free(dtb_nuevo);
					return dtb_nuevo;
				}
			}

			dtb_nuevo->archivosAbiertos = list_create();
			DTB_agregar_archivo(dtb_nuevo, 0, path);

			return dtb_nuevo;
		}

        void desbloquear_dtb_dummy(DTB* dtb_nuevo)
        {
			DTB* dtb_dummy = malloc (sizeof(DTB));
			ArchivoAbierto* escriptorio = DTB_obtener_escriptorio(dtb_nuevo);
			dtb_dummy = crear_dtb(dtb_nuevo->gdtPID, escriptorio->path, 0);
			list_add(lista_plp, dtb_dummy);
		}
		
		void ejecutar(char* path)
        {
			DTB* dtb_nuevo = crear_dtb(0, path, 1);
			list_add(lista_nuevos, dtb_nuevo);
			desbloquear_dtb_dummy(dtb_nuevo);
		}

		void liberar_archivo_abierto(void *archivo)
		{
			free(((ArchivoAbierto *)archivo)->path);
			free(archivo);
		}

		void liberar_dtb(void *dtb)
        {
			list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
			free(dtb);
		}

        before
        {
            lista_nuevos = list_create();
            lista_plp = list_create();
        } end

        after 
        {
            list_destroy_and_destroy_elements(lista_nuevos,liberar_dtb);
            list_destroy_and_destroy_elements(lista_plp, liberar_dtb);
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
            should_int(dtb1->PC) be equal to (1);
            should_int(dtb1->flagInicializacion) be equal to (1);

            ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb1);
            should_string(escriptorio->path) be equal to ("path1");
            should_int(escriptorio->cantLineas) be equal to (0);
		}
        end

		it("ejecutar path agrega dtb_dummy a lista plp") {
            should_int(list_size(lista_plp)) be equal to (0);
			ejecutar(path_escriptorio1);
            should_int(list_size(lista_plp)) be equal to (1);
		}
        end

        it("ejecutar path crea dtb_dummy con los campos correctos")
        {
            ejecutar(path_escriptorio1);

            DTB *dtb_dummy = list_get(lista_plp, 0);
			should_int(dtb_dummy->gdtPID) be equal to (1);
            should_int(dtb_dummy->PC) be equal to (1);
            should_int(dtb_dummy->flagInicializacion) be equal to (0);
            
            ArchivoAbierto *escriptorio = DTB_obtener_escriptorio(dtb_dummy);
            should_string(escriptorio->path) be equal to ("path1");
            should_int(escriptorio->cantLineas) be equal to (0);
        }
        end
    }
    end
}