LIBS = -lcommons -lcspecs
TESTS_PATH = /home/utnso/workspace/tp-2018-2c-Nene-Malloc/cpu/tests 
all: abrir asignar flush close crear
abrir:
	gcc test_abrir.c -I$(TESTS_PATH) -o abrir $(LIBS)
asignar:
	gcc test_asignar.c -I$(TESTS_PATH) -o asignar $(LIBS)
flush:
	gcc test_flush.c -I$(TESTS_PATH) -o flush $(LIBS)
close:
	gcc test_close.c -I$(TESTS_PATH) -o close $(LIBS)
crear:
	gcc test_crear.c -I$(TESTS_PATH) -o crear $(LIBS)

clean:
	-rm -f *.o
	-rm -f abrir asignar flush close crear
