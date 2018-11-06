LIBS = -lcommons -lcspecs
TESTS_PATH = /home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/tests 
plp:
	gcc test_plp.c -I$(TESTS_PATH) -o plp $(LIBS) 

mutex:
	gcc test_mutex.c -I$(TESTS_PATH) -o mutex $(LIBS)
clean:
	-rm -f *.o
	-rm -f plp mutex
