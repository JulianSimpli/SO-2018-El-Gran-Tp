TARGET = safa
LIBS = -lcommons -lreadline
program:
	gcc -pthread safa.c -I/home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/src -o $(TARGET) $(LIBS) planificador.c ../../Bibliotecas/sockets.c

clean:
	-rm -f *.o
	-rm -f $(TARGET)
