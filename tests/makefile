LIBS = -lcommons -lcspecs
all: program1 program2

program1:
	gcc test_dtb_deserializar.c -I/home/utnso/workspace/tp-2018-2c-Nene-Malloc/tests -o deserializar $(LIBS)

program2:
	gcc test_dtb_serializar.c -I/home/utnso/workspace/tp-2018-2c-Nene-Malloc/tests -o serializar $(LIBS)

clean:
	-rm -f *.o
	-rm -f $(TARGET)
