#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <cspecs/cspec.h>
#include "../src/mdj.h"

context(test_leer_filesystem) {

	describe("MDJ puede leer el filesystem inicial") {

		before
		{
			logger = log_create("MDJTEST.log", "TEST", true, LOG_LEVEL_INFO);
			config = config_create("MDJTEST.config");
			/*
			char *mnt_config = config_get_string_value(config, "PUNTO_MONTAJE");
			mnt_path = malloc(strlen(mnt_config) + 1);
			strcpy(mnt_path, mnt_config);
			*/
			crear_bitarray();
		} end

		after
		{
			log_destroy(logger);
			config_destroy(config);
			/*
			bitarray_destroy(bitarray);
			*/
		} end

		it("should find a file in the mount point")
		{
			int used = bitarray_test_bit(bitarray, 1);
			should_int(used) be equal to(1);
		} end

		it("should report with blocks are used")
		{
			int max = bitarray_get_max_bit(bitarray);
			int i = 1;
			printf("\n");
			for(i = 1; i < max; i++) {
				printf("%d", bitarray_test_bit(bitarray, i));
				if (i % 32 == 0 && i != 0) printf("\n");
			}
			printf("\n");
			
			should_bool(true) be equal to(true);
		} end

	}
	end
}
