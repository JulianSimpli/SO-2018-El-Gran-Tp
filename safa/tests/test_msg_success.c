#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"

context(msg_success)
{
    describe("Llegan mensajes de exito de ejecucion de dummy o dtb")
    {
        t_list *lista_nuevos;
        t_list *lista_plp;
        DTB *dtb;

        // void dummy_succes_mock()
        // {
        //     Paquete *paquete = sizeof(Paquete);

        //     cargar_header(&paquete, tamanio_payload, DUMMY_SUCCES, CPU);
        // }

        it("Llenar")
        {
            should(false) be truthy;
        }
        end
    }
    end
}