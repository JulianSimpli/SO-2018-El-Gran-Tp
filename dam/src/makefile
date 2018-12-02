TARGET = dam
LIBS = -lcommons -lreadline ../../Bibliotecas/sockets.c ../../Bibliotecas/dtb.c ../../Bibliotecas/helper.c


program:
	gcc -pthread dam.c -o $(TARGET) $(LIBS)
clean:
	-rm -f *.o
	-rm -f $(TARGET)
