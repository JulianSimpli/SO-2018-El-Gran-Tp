#include <cspecs/cspec.h>
#include "../src/cpu.h"

context(test_primitivas) {

	describe("CPU puede parsear") {

		char *primitiva = "flush /equipos/Racing.txt";

		it("Puede parsear flush") {
			bool parseo = interpretar(primitiva);
			should_bool(parseo) be truthy;
		} end

	} end

}
