#include <cspecs/cspec.h>
#include "../Bibliotecas/dtb.h"

context(Recursos_safa)
{
    describe("Manejo de recursos en SAFA con Wait y Signal")
    {   
        char *test = "Kappa";

        void *serializar_string(string, int *tamanio_desplazado)
        {
            void *payload = sizeof(string);
            u_int32_t len_string = strlen(string);

            memcpy(payload + *desplazamiento, &len_string, sizeof(len_string));
            *desplazamiento += sizeof(len_string);
            payload = realloc(payload, len_string);
            memcpy(payload + *desplazamiento, string, len_string);
            *desplazamiento += len_string;

            return payload;
        }

        char *deserializar_string(void *payload)
        {
            u_int32_t len_string = 0;
            memcpy(&len_string, payload, u_int32_t);
            char *string = malloc(sizeof(len_string) + 1);
            memcpy(string, payload, len_string);
            *string + len_string = '\0';

            return string;
        }
        before
        {

        }
        end

        after
        {
            free(test);
        }
        end

        it("Encuentra un recurso en la lista de recursos")
        {
            should_bool(true) be falsey;
        }
        end

        it("Agrega un recurso a la lista si no existe previamente")
        {
            should_bool(false) be truthy;
        }
        end
        
        it("Serializa y deserializa un char*")
        {
            void *payload;
            int desplazamiento = 0;
            payload = string_serializar(test, &desplazamiento);
            should_int(desplazamiento) be equal to (9)

            char *deserializado = string_deserializar(payload);
        }
        end

        it("Bloquea DTB tras wait con sem en 0")
        {
            should_bool(false) be truthy;
        }
        end

        it("NO bloquea DTB tras wait con sem en 1")
        {
            should_bool(false) be truthy;
        }
        end

        it("Desbloquea DTB tras signal con sem en -1")
        {
            should_bool(false) be truthy;
        }
        end
    }
    end
}