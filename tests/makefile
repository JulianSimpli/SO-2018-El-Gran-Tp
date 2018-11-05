LIBS = -lcommons -lcspecs ../Bibliotecas/dtb.c
all: program1 program2 dtb

program1:
	gcc test_dtb_deserializar.c -o deserializar $(LIBS)

program2:
	gcc test_dtb_serializar.c -o serializar $(LIBS)

dtb:
	gcc test_dtb.c -o dtb $(LIBS)

clean:
	-rm -f *.o
	-rm -f serializar deserializar dtb
