TARGET = mdj
LIBS = -lcommons -lreadline
program:
	gcc -pthread mdj.c -I/home/tp-2018-2c-Nene-Malloc/mdj/src -o $(TARGET) $(LIBS) ../../Bibliotecas/sockets.c ../../Bibliotecas/helper.c
clean:
	-rm -f *.o
	-rm -f $(TARGET)
