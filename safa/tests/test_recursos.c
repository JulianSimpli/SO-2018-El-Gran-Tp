#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <stdio.h>

typedef struct {
	char *id;
	u_int32_t semaforo;
	t_list *pid_bloqueados;
}__attribute__((packed)) t_recurso;


context(Recursos_safa)
{
    describe("Manejo de recursos en SAFA")
    {   
        char *test;
        t_recurso *recurso;
        t_list *lista_recursos_global;

        void *string_serializar(char *string, int *desplazamiento)
        {
            u_int32_t len_string = strlen(string);
            void *serializado = malloc(sizeof(len_string) + len_string);

            memcpy(serializado + *desplazamiento, &len_string, sizeof(len_string));
            *desplazamiento += sizeof(len_string);
            memcpy(serializado + *desplazamiento, string, len_string);
            *desplazamiento += len_string;

            return serializado;
        }

        char *string_deserializar(void *data, int *desplazamiento)
        {         
            u_int32_t len_string = 0;
            memcpy(&len_string, data + *desplazamiento, sizeof(len_string));
            *desplazamiento += sizeof(len_string);
            
            char *string = malloc(len_string + 1);
            memcpy(string, data + *desplazamiento, len_string);
            *(string+len_string) = '\0';
            *desplazamiento = *desplazamiento + len_string + 1;

            return string;
        }

        t_recurso *recurso_crear(char *id_recurso, int valor_inicial)
        {
            t_recurso *recurso = malloc(sizeof(t_recurso));
            recurso->id = malloc(strlen(id_recurso) + 1);
            strcpy(recurso->id, id_recurso);
            recurso->semaforo = valor_inicial;
            recurso->pid_bloqueados = list_create();

            list_add(lista_recursos_global, recurso);

            return recurso;
        }

        void recurso_liberar(t_recurso *recurso)
        {
            free(recurso->id);
            list_destroy_and_destroy_elements(recurso->pid_bloqueados, free);
            free(recurso);
        }

        bool coincide_id(void *recurso, char *id)
        {
            return !strcmp(((t_recurso *)recurso)->id, id);
        }

        t_recurso *recurso_encontrar(char* id_recurso)
        {
            bool comparar_id(void *recurso)
            {
                return coincide_id(recurso, id_recurso);
            }

            return list_find(lista_recursos_global, comparar_id);
        }
     
        before
        {
            test = "Kappa";
            lista_recursos_global = list_create();
            recurso = recurso_crear(test, 1);
        }
        end

        after
        {
            list_destroy_and_destroy_elements(lista_recursos_global, recurso_liberar);
        }
        end

        it("Crea recurso con parametros correctos. Lo agrega a lista_recursos_global")
        {
            should_string(recurso->id) be equal to ("Kappa");
            should_int(list_size(recurso->pid_bloqueados)) be equal to (0);
            should_int(recurso->semaforo) be equal to (1);
            
            should_int(list_size(lista_recursos_global)) be equal to (1);
            t_recurso *rec = list_get(lista_recursos_global, 0);
            should_string(rec->id) be equal to ("Kappa");
            should_int(list_size(rec->pid_bloqueados)) be equal to (0);
            should_int(rec->semaforo) be equal to (1);

        }
        end

        it("Encuentra un recurso a partir de un string")
        {
            should_bool(coincide_id(recurso, "Kappa")) be truthy;
            
            t_recurso *rec1 = recurso_encontrar("Kappa");
            should_bool(coincide_id(rec1, test)) be truthy; // test = "Kappa";
            should_string(rec1->id) be equal to ("Kappa");
            should_int(rec1->semaforo) be equal to (1);
            should_int(list_size(rec1->pid_bloqueados)) be equal to (0);

            t_recurso *rec2 = recurso_encontrar("Keepo");
            should_ptr(rec2) be equal to (NULL);
        }
        end
        
        it("Serializa y deserializa un char*")
        {
            int desplazamiento = 0;
            void *payload;
            payload = string_serializar(test, &desplazamiento);
            should_int(desplazamiento) be equal to (9);

            int desp2 = 0;
            char *deserializado = string_deserializar(payload, &desp2);
            should_int(desp2) be equal to (10);
            should_string(deserializado) be equal to ("Kappa");
            
            free(payload);
        }
        end
    }
    end
}