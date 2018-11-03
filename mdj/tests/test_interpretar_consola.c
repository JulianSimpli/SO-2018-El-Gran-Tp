/*
 * Copyright (C) 2012 Sistemas Operativos - UTN FRBA. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <commons/bitarray.h>
#include <commons/log.h>
#include <stdlib.h>
#include <string.h>
#include <cspecs/cspec.h>
#include "../../src/mdj.h"

/**
 * TODO: This tests are 8-BIT dependant. We should think of a way to abstract them.
 * TODO: Can we abstract the LSB and MSB tests so they get unified?
 */
context (test_interpretar_consola) {

    describe ("MDJ interpreta consola") {

	t_log *logger;
        t_bitarray *bitarray;

        int interpretar(char* linea) {
		int i = 0, existe = 0;
		size_t command_size = sizeof(commands) / sizeof(commands[0]) - 1;
		for (i; i < command_size; i++) {
			if (!strcmp(commands[i].name, linea)) {
				existe = 1;
				log_info(logger, commands[i].doc);
				commands[i].func(linea);
				break;
			}
		}

		if (!existe) {
			error_show("No existe el comando %s\n", linea);
		}

		return 0;
	}

        before {
	    logger = log_create("TEST.log", "TEST", false, LOG_LEVEL_DEBUG);
        } end

        after {
            bitarray_destroy(bitarray);
            log_destroy(logger);
        } end

        it ("should interpret CD") {
	    ContentHeader *mensaje = crear_mensaje(1);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should interpret CAT") {
	    ContentHeader *mensaje = crear_mensaje(2);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should interpret LS") {
	    ContentHeader *mensaje = crear_mensaje(3);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should interpret MD5") {
	    ContentHeader *mensaje = crear_mensaje(4);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should interpret UNKNOWN COMMAND") {
	    	ContentHeader *mensaje = crear_mensaje(4);
		    ContentHeader *respuesta = interpretar_mensaje(mensaje);
            should_int( respuesta->id ) be equal to(200);
        } end

    } end

}
