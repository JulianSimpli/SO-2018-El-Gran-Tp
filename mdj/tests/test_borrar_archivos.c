#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <cspecs/cspec.h>
#include "../src/mdj.h"
#include <readline/readline.h>
#include <readline/history.h>

context(test_interpretar_archivos)
{

	describe("MDJ maneja metadata de archivos"){

		char *ruta = "/home/utnso/fifa-examples/fifa-checkpoint/Archivos/scripts/checkpoint.escriptorio"; 
		Paquete *paquete;
		t_log *logger;
		t_config *config;
		t_bitarray *bitarray;

		Paquete *mock_borrar()
		{
			//Borra un escriptorio cuando en realidad creo que nunca pasaria
			paquete = malloc(sizeof(Paquete));
			int desplazamiento = 0;
			void *serializado = string_serializar(ruta, &desplazamiento);
			paquete->header = cargar_header(desplazamiento, BORRAR_ARCHIVO, ELDIEGO);
			paquete->Payload = serializado;
			return paquete;
		}

		before {
			printf("asd");
			logger = log_create("MDJTEST.log", "TEST", true, LOG_LEVEL_INFO);
			config = config_create("MDJTEST.config");
			log_debug(logger, "asd");
			//system("cd /home/utnso/fifa-examples && git checkout fifa-checkpoint/Archivos/scripts/checkpoint.escriptorio");
			//system("cd /home/utnso/fifa-examples && git checkout fifa-checkpoint/Metadata/Bitmap.bin");
			log_debug(logger, "asd");
			cargar_bitarray();
			mock_borrar();
		} end

		after {
			log_destroy(logger);
			config_destroy(config);
			bitarray_destroy(bitarray);
		} end


it("Deberia liberarlo en el bitarray")
{
	int ocupado = bitarray_test_bit(bitarray, 1);
	should_int(ocupado) be equal to(1);
	//borrar_archivo(paquete);
	should_int(bitarray_test_bit(bitarray, 1)) be equal to(0);
	should_int(bitarray_test_bit(bitarray, 2)) be equal to(0);
	should_int(bitarray_test_bit(bitarray, 3)) be equal to(0);
	should_int(bitarray_test_bit(bitarray, 4)) be equal to(0);
	should_int(bitarray_test_bit(bitarray, 5)) be equal to(0);
}
end
}
end
}
