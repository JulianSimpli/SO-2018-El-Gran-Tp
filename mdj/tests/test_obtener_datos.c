#include <cspecs/cspec.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/mdj.h"

context(test_obtener_datos) {

	describe("MDJ obtiene datos del file system") {

		char *primitiva = "crear /equipos/equipo1.txt 5";
		char *resto = "abrir /equipos/equipo";
		char *entero = "crear /equipos/equipo1.txt 5\nabrir /equipos/equipo1.txt\nasignar /equipos/equipo1.txt 1 Rasing\nasignar /equipos/equipo1.txt 1 Jugadores: Centurion\nflush /equipos/equipo1.txt\nclose /equipos/equipo1.txt\n\n";

		char *get_block_full_path(int bloque) {
			char *ubicacion = malloc(10);
			sprintf(ubicacion, "%d.bin", bloque);
			return ubicacion;
		}

		char *leer_bloque(int bloque, int desde, int cantidad) {
			char *ret = malloc(cantidad);
			char *ubicacion = get_block_full_path(bloque);
			FILE *bloque_abierto = fopen(ubicacion, "rb");

			if (bloque_abierto == NULL)
				perror("Al leer el bloque: ");
			
			fseek(bloque_abierto, desde, SEEK_SET);
			fread(ret, 1, cantidad, bloque_abierto); //man fgets devuelve 1 menos
			fclose(bloque_abierto);
			free(ubicacion);
			return ret;
		}

		before {
			blocks_path = "";
		} end

		after {

		} end

		it("Logra leer primer primitiva") {
			should_int(strlen(primitiva)) be equal to(28);
			char *data = leer_bloque(0, 0, 28);
			data[28] = '\0';
			should_int(strlen(data)) be equal to(28);
			should_string(data) be equal to(primitiva);
		} end

		it("Logra leer hasta el maximo del bloque") {
			char *data = leer_bloque(0, 29, 21);
			data[21] = '\0';
			should_int(strlen(data)) be equal to(21);
			should_string(data) be equal to(resto);
		} end

		it("Puede leer bloque entero") {
			char *data = leer_bloque(0, 0, 50);
			data[50] = '\0';
			should_int(strlen(data)) be equal to(50);
		} end

		it("Puede leer lo que le falta sin traer caracteres extra") {
			char *data = leer_bloque(4, 0, 1);
			data[1] = '\0';
			should_int(strlen(data)) be equal to(1);
		} end

		it("Puede devolver los primeros cuatro bloques") {
			char *data = malloc(200 +2);
			strcat(data, leer_bloque(0, 0, 50));
			strcat(data, leer_bloque(1, 0, 50));
			strcat(data, leer_bloque(2, 0, 50));
			strcat(data, leer_bloque(3, 0, 50));
			strcat(data, leer_bloque(4, 0, 1));
			data[201] = '\0';
			should_int(strlen(data)) be equal to(201);
			should_string(data) be equal to(entero);
		} end


	} end

}
