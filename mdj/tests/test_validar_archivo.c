#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <cspecs/cspec.h>
#include <string.h>
#include <dirent.h>

context(test_interpretar_archivos)
{

	describe("MDJ maneja metadata de archivos")
	{

		it("Deberia poder conseguir el nombre del escriptorio")
		{
			char *search_file = "/scripts/simple.escriptorio";
			DIR *dir;
			struct dirent *ent;

			dir = opendir("/home/utnso/fifa-examples/fifa-entrega/Archivos");
			ent = readdir(dir);
			perror("opendir: ");

			should_string(ent->d_name) be equal to("scripts");

			//DIR *dir2;
			//struct dirent *ent2;
			dir = opendir("/home/utnso/fifa-examples/fifa-entrega/Archivos/scripts");
			perror("opendir: ");
			if (dir == NULL)
				should_bool(false) be truthy;

			ent = readdir(dir);
			char *r = malloc(strlen(ent->d_name) + 1);
			strcpy(r, ent->d_name);
			should_string(r) be equal to("share");

			FILE *f = fopen("/home/utnso/0.bin", "rb");
			fseek(f, 0, SEEK_END);
			int size = ftell(f);

			should_int(size) be equal to(50);

			/*
			strtok(search_file, "/");
			char *split = "scripts";
			if (split == NULL)
				should_bool(false) be truthy;
			should_string(split) be equal to("scripts");
			*/
		} end

		it ("Deberia decir el tamanio del archivo") 
		{
			FILE *f = fopen("0.bin", "rb");
			fseek(f, 0, SEEK_END);
			int size = ftell(f);

			should_int(size) be equal to(50);
		} end
	}
	end
}
