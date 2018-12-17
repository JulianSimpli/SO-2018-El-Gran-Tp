TARGET = safa
LIBS = -lcommons -lreadline
program:
	gcc -pthread safa.c -I/home/utnso/workspace/tp-2018-2c-Nene-Malloc/safa/src -o $(TARGET) $(LIBS) planificador.c ../../Bibliotecas/sockets.c ../../Bibliotecas/dtb.c ../../Bibliotecas/helper.c ../../Bibliotecas/loggers.c
 ##-Wall -ggdb3
clean:
	-rm -f *.o
	-rm -f $(TARGET)
