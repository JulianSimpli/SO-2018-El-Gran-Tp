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
context (test_interpretar_mensaje) {

    describe ("MDJ interpreta mensajes") {

	t_log *logger;
        t_bitarray *bitarray;

	typedef struct {
	  int id;
	  int len;
	} __attribute__((packed)) ContentHeader;

	ContentHeader * interpretar_mensaje(ContentHeader *mensaje) {
		// id = 400 and above will be for error reporting
		ContentHeader *respuesta = (ContentHeader *) malloc(sizeof(ContentHeader));
		respuesta->id = 200;
		respuesta->len = 0; //Default to empty answer
	
		int id = mensaje->id;

		switch(id) {
			case 1:
				log_info(logger, "Validar archivo");
				break;
			case 2:
				log_info(logger, "Crear archivo");
				break;
			case 3:
				log_info(logger, "Obtener datos");
				break;
			case 4:
				log_info(logger, "Guardar datos");
				break;
			default:
				log_warning(logger, "Mensaje con id = %d  no tiene respuesta posible", id);
				respuesta->id = 404; //Function not found 
				break;
		}

		return respuesta;
	}

	ContentHeader * crear_mensaje(int id)
	{
	    ContentHeader *mensaje = (ContentHeader *) malloc(sizeof(ContentHeader));
	    mensaje->id = id;
	    return mensaje;
	}

        before {
	    logger = log_create("TEST.log", "TEST", false, LOG_LEVEL_DEBUG);
        } end

        after {
            bitarray_destroy(bitarray);
            log_destroy(logger);
        } end

        it ("should return success for VALIDAR ARCHIVO") {
	    ContentHeader *mensaje = crear_mensaje(1);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should return success for CREAR ARCHIVO") {
	    ContentHeader *mensaje = crear_mensaje(2);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should return success for OBTENER DATOS") {
	    ContentHeader *mensaje = crear_mensaje(3);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should return success for GUARDAR DATOS") {
	    ContentHeader *mensaje = crear_mensaje(4);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(200);
        } end

        it ("should return error for UNKNOWN") {
	    ContentHeader *mensaje = crear_mensaje(-1);
	    ContentHeader *respuesta = interpretar_mensaje(mensaje);

            should_int( respuesta->id ) be equal to(404);
        } end

    } end

}
