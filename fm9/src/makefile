TARGET = fm9
LIBS = -lcommons -lreadline -lm
program:
	gcc -pthread fm9.c -o $(TARGET) $(LIBS) ../../Bibliotecas/sockets.c ../../Bibliotecas/dtb.c ../../Bibliotecas/helper.c
clean:
	-rm -f *.o
	-rm -f $(TARGET)
