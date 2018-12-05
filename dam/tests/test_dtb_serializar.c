#include <cspecs/cspec.h>
#include "../src/dam.h"

context(test_dam_contesta_a_safa) {

	describe("contesta a safa") {

		it("Contesta serializar y deserializar dtb entero") {
			should_string(b->path) be equal to("otro_ejemplo.txt");
		} end

	} end

}
