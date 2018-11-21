TARGET = cpu
LIBS = -lcommons -lreadline ../../Bibliotecas/dtb.c ../../Bibliotecas/sockets.c 
program:
	gcc -pthread cpu.c -o $(TARGET) $(LIBS)
clean:
	-rm -f *.o
	-rm -f $(TARGET)
