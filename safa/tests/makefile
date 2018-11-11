LIBS = -lcommons -lcspecs
TESTS_PATH = /home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/tests 

all:
	gcc test_plp.c -I$(TESTS_PATH) -o plp $(LIBS) 
	gcc test_mutex.c -I$(TESTS_PATH) -o mutex $(LIBS)
	gcc test_handshakes.c -I$(TESTS_PATH) -o handshakes $(LIBS)
	gcc test_msg_success.c -I$(TESTS_PATH) -o success $(LIBS)
	gcc test_ejecutar.c -I$(TESTS_PATH) -o ejecutar $(LIBS)
	gcc test_finalizar.c -I$(TESTS_PATH) -o finalizar $(LIBS)

plp:
	gcc test_plp.c -I$(TESTS_PATH) -o plp $(LIBS) 

mutex:
	gcc test_mutex.c -I$(TESTS_PATH) -o mutex $(LIBS)

handshakes:
	gcc test_handshakes.c -I$(TESTS_PATH) -o handshakes $(LIBS)

success:
	gcc test_msg_success.c -I$(TESTS_PATH) -o success $(LIBS)

ejecutar:
	gcc test_ejecutar.c -I$(TESTS_PATH) -o ejecutar $(LIBS)

finalizar:
	gcc test_finalizar.c -I$(TESTS_PATH) -o finalizar $(LIBS)

clean:
	-rm -f *.o
	-rm -f plp mutex handshakes success ejecutar finalizar
