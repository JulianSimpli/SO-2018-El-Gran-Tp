TARGET = mdj
LIBS = -lcommons -lreadline -lcrypto
program:
	gcc -pthread mdj.c -I/home/tp-2018-2c-Nene-Malloc/mdj/src -o $(TARGET) $(LIBS) ../../Bibliotecas/sockets.c ../../Bibliotecas/helper.c ../../Bibliotecas/loggers.c
clean:
	-rm -f *.o
	-rm -f $(TARGET)
