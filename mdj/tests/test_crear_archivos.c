#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <cspecs/cspec.h>
#include "../src/mdj.h"

context(test_interpretar_archivos)
{

	describe("MDJ maneja metadata de archivos")
	{

		typedef struct file_metadata
		{
			char *nombre;
			int tamanio;
			t_list* bloques;
		} FILE_METADATA;

		void free_metadata(FILE_METADATA *fm)
		{
			list_destroy(fm->bloques);
			free(fm);
		}

		/**
		 * Busca en la config el tamanio 
		 * Verifica si entra entero o si require otro bloque
		 * El cual va tener fragmentacion interna
		 */
		int calculate_blocks(int tamanio)
		{
			int bloques = tamanio / tamanio_bloques;
			return tamanio % tamanio_bloques == 0 ? bloques : bloques + 1;
		}

		char *FM_get_bloques_as_string(FILE_METADATA * fm)
		{
			char *bloques = malloc(256);
			int i;
			sprintf(bloques, "[%d", (int)(intptr_t) list_get(fm->bloques, 0));
			
			for (i = 1; i < list_size(fm->bloques); i++)
			{
				sprintf(bloques, "%s,%d", bloques, (int)(intptr_t) list_get(fm->bloques, i));
			}

			return strcat(bloques, "]");
		}

		/**
		 * Devuelve en una lista los bloques que tiene disponible
		 */
		t_list* get_available_blocks(int blocks)
		{
			t_list *free_blocks = list_create();
			int i;
			for(i = 1; blocks != 0  && i < cantidad_bloques; i++)
			{
				//El bitarray esta desocupado en esa posicion
				if(bitarray_test_bit(bitarray, i)) {
					log_debug(logger, "Bloque libre: %d", i);
					list_add(free_blocks, (void*)(__intptr_t)i);
					blocks--;
				}
			}			

			if (blocks != 0)
				return list_create(); //Returns empty list

			return free_blocks;
		}

		/**
		 * Calcula su espacio en base a bloques 
		 */
		t_list* get_space(int size)
		{
			int blocks = calculate_blocks(size);
			log_debug(logger, "Necesita %d bloques", blocks);
			return get_available_blocks(blocks);
		}

		/**
		 * Retorna:
		 * 0 en caso positivo
		 * 1 en caso negativo 
		 */
		int FM_save_to_file(FILE_METADATA * fm)
		{
			FILE *f = fopen(fm->nombre, "w+");

			if (f == NULL)
			{
				log_error(logger, "%s", strerror(errno));
				return 1;
			}

			fprintf(f, "%s%d\n", "TAMANIO=", fm->tamanio);
			fprintf(f, "%s%s\n", "BLOQUES=", FM_get_bloques_as_string(fm));

			fclose(f);
			return 0;
		}

		void cargar_fs_metadata()
		{
			char *aux = config_get_string_value(config, "PUNTO_MONTAJE");
			char *metadata = "/Metadata/Metadata.bin";
			metadata_path = malloc(strlen(aux) + strlen(metadata) + 1);
			strcpy(metadata_path, aux);
			strcat(metadata_path, metadata);
			char *blocks = "/Bloques";
			blocks_path = malloc(strlen(aux) + strlen(blocks) + 1);
			strcpy(blocks_path, aux);
			strcat(blocks_path, blocks);
			t_config *fs_config = config_create(metadata_path);
			cantidad_bloques = config_get_int_value(fs_config, "CANTIDAD_BLOQUES");
			tamanio_bloques = config_get_int_value(fs_config, "TAMANIO_BLOQUES");
			config_destroy(fs_config);
		}

		before
		{
			logger = log_create("MDJTEST.log", "TEST", true, LOG_LEVEL_INFO);
			config = config_create("MDJTEST.config");

			//Tengo el tamanio de bloque en la metadata del filesystem
			cargar_fs_metadata();

			char data[] = { 0, 0, 0 };
            bitarray = bitarray_create_with_mode(data, cantidad_bloques / 8, LSB_FIRST);
		}
		end

		after
		{
			log_destroy(logger);
			bitarray_destroy(bitarray);
		}
		end

		it("should transform int array into , separated char *")
		{
			FILE_METADATA *fm = (FILE_METADATA *)malloc(sizeof(FILE_METADATA));
			fm->tamanio = 4;
			fm->bloques = list_create();
			list_add(fm->bloques, (void*) 1);
			list_add(fm->bloques, (void*) 2);
			list_add(fm->bloques, (void*) 3);
			char *bloques = FM_get_bloques_as_string(fm);
			should_string(bloques) be equal to("[1,2,3]");
			free_metadata(fm);
		}
		end

		it("should save metadata in a file")
		{
			FILE_METADATA *fm = (FILE_METADATA *)malloc(sizeof(FILE_METADATA));
			char *nombre = "testfile";
			fm->nombre = malloc(strlen(nombre) + 1);
			memcpy(fm->nombre, nombre, strlen(nombre) + 1);
			fm->tamanio = 4;
			fm->bloques = list_create();
			list_add(fm->bloques, (void*) 1);
			list_add(fm->bloques, (void*) 2);
			list_add(fm->bloques, (void*) 3);
			FM_save_to_file(fm);
			should_string(fm->nombre) be equal to("testfile");
			free_metadata(fm);
		}
		end

		it("should create a .bin block")
		{
			int index = 2;
			char data[] = { 0, 0, 0 };
            bitarray = bitarray_create_with_mode(data, cantidad_bloques / 8, LSB_FIRST);

			int success = create_block(index);
			should_int(success) be equal to(0);
		}
		end
		
		it("should set bits that are now used")
		{
			int index = 2;
			char data[] = { 0, 0, 0 };
            bitarray = bitarray_create_with_mode(data, cantidad_bloques / 8, LSB_FIRST);
			should_bool(bitarray_test_bit(bitarray, index)) be equal to(false);

			create_block(index);
			bool bitset = bitarray_test_bit(bitarray, index);
			should_bool(bitset) be equal to(true);
		}
		end
/*
		it("should refuse to save a file because is out of space")
		{
			Mensaje *respuesta = (Mensaje *)malloc(sizeof(Mensaje));
			respuesta->paquete.header.emisor = MDJ;

			t_list *free_blocks = get_space(250);

			if (list_size(free_blocks) == 0) {
				respuesta->paquete.header.tipoMensaje = ERROR;
				respuesta->paquete.Payload = (void*)50002;
			}

			should_int(respuesta->paquete.header.tipoMensaje) be equal to (50002);
		}
		end

		it("should successfully save a file and create his .bin blocks")
		{
			Mensaje *respuesta = (Mensaje *)malloc(sizeof(Mensaje));
			respuesta->paquete.header.emisor = MDJ;

			t_list *free_blocks = get_space(250);

			if (list_size(free_blocks) == 0) {
				respuesta->paquete.header.tipoMensaje = ERROR;
				respuesta->paquete.Payload = (void*)50002;
			}

			respuesta->paquete.header.tipoMensaje = SUCCESS;

			should_int(respuesta->paquete.header.tipoMensaje) be equal to (SUCCESS);
		}
		end
		*/
	}
	end
}
