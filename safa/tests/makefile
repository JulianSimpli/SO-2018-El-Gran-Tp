LIBS = -lcommons -lcspecs
TESTS_PATH = /home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/tests 

all:
	gcc test_plp.c -I$(TESTS_PATH) -o plp $(LIBS) 
	gcc test_mutex.c -I$(TESTS_PATH) -o mutex $(LIBS)
	gcc test_handshakes.c -I$(TESTS_PATH) -o mensajes $(LIBS)
	gcc test_ejecutar.c -I$(TESTS_PATH) -o ejecutar $(LIBS)

plp:
	gcc test_plp.c -I$(TESTS_PATH) -o plp $(LIBS) 

mutex:
	gcc test_mutex.c -I$(TESTS_PATH) -o mutex $(LIBS)

mensajes:
	gcc test_handshakes.c -I$(TESTS_PATH) -o mensajes $(LIBS)

ejecutar:
	gcc test_ejecutar.c -I$(TESTS_PATH) -o ejecutar $(LIBS)

clean:
	-rm -f *.o
	-rm -f plp mutex mensajes ejecutar
