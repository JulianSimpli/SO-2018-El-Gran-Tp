#include <commons/log.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <commons/string.h>
#include <string.h>
#include <cspecs/cspec.h>
#include "../../Bibliotecas/sockets.h"

context(test_primitives)
{

	describe("CPU understand the open primitive") {

		char *message = "abrir /equipos/Racing.txt";
		int DAM_SOCKET = 10;
		int SAFA_SOCKET = 11;

		/**
		 * This will depend if a pcb has a list or a char**
		 */
		t_list *get_open_files(void* pcb)
		{
			t_list *files = list_create();
			list_add(files, (void*) "/equipos/Racing.txt");
			return files;
		}

		bool find_file(t_list *files, char *path)
		{
			bool match_path_name(void *element)
			{
				return strcmp(path, (char *)element) == 0;
			}

			return list_any_satisfy(files, match_path_name);
		}

		/**
		 * Verificar si dicho archivo ya se encuentra abierto por el G.DT.
		 * Para esto se debe validar en las estructuras administrativas del DTB.
	 	 * En caso que ya se encuentre abierto, se procederá con la siguiente primitiva.
		 */
		bool is_file_already_open(char *path)
		{
			t_list *files = get_open_files((void*) NULL);

			return find_file(files, path);
		}

		/**
		 * Returns true if it can continue to the next primitive
		 * Returns false otherwise
		 */
		bool handle_abrir(char **words)
		{
			char *parameter = words[1];

			if (is_file_already_open(parameter)) {
				return true;
			}

			//En caso que no se encuentre abierto, se enviará una solicitud a El Diego
			//para que traiga desde el MDJ el archivo requerido.

			//send_message(, ELDIEGO);

			//Cuando se realiza esta operatoria, el CPU desaloja al DTB indicando a S-AFA
			//que el mismo está esperando que El Diego cargue en FM9 el archivo deseado.
			//Esta operación hace que S-AFA bloquee el G.DT.

			//send_message(, SAFA);			

			//Cuando El Diego termine la operatoria de cargarlo en FM9,
			//avisará a S-AFA los datos requeridos para obtenerlo de FM9
			//para que pueda desbloquear el G.DT.

			return false;
		}

		it("Can parse command from message"){
			char **words = string_split(message, " ");
			char *primitive = words[0];
			should_string(primitive) be equal to("abrir");
		}
		end

		it("Can parse parameters from message"){
			char **words = string_split(message, " ");
			char *parameter = words[1];
			should_string(parameter) be equal to("/equipos/Racing.txt");
		}
		end

		it("Can find a matching path on a list"){
			t_list *files = list_create();
			list_add(files, (void*) "test");
			bool match = find_file(files, "test");
			should_bool(match) be truthy;
		}
		end

		it("Can check if a file is already open in a pcb"){
			char **words = string_split(message, " ");
			char *parameter = words[1];
			bool status = is_file_already_open(parameter);

			should_bool(status) be truthy;
		}
		end

		it("Has the file already open and continues to the next primitive"){
			char **words = string_split(message, " ");

			bool status = handle_abrir(words);

			should_bool(status) be truthy;
		}
		end

		it("Has to request others to open the file"){
			char *message = "abrir /equipos/Independiente.txt";
			char **words = string_split(message, " ");
			bool status = handle_abrir(words);
			should_bool(status) be falsey;
		}
		end

		/**
		 * Sends to DAM the path to the file that has to be open
		 * and the pid of the pcb that is getting blocked
		 */
		Mensaje *message_to_dam()
		{
			Mensaje *message = malloc(sizeof(Mensaje)); 
			message->socket = DAM_SOCKET; 
			message->paquete.header.tipoMensaje = ABRIR_ARCHIVO;
			message->paquete.header.emisor = CPU;

			char *parameter = "/equipos/Banfield.txt";
			size_t len = strlen(parameter);
	//			uint32_t pid = 1;
			size_t size = sizeof(uint32_t);
			message->paquete.header.tamPayload = len;
			//message->paquete.header.tamPayload = len + size;
			message->paquete.Payload = malloc(len);
			//message->paquete.Payload = malloc(len + size);
			memmove(message->paquete.Payload, parameter, len);
		//	memmove(message->paquete.Payload, &pid, size);
			return message;
		}

		it("Can make the proper message to DAM"){
			Mensaje *message = message_to_dam(); 

			size_t len = message->paquete.header.tamPayload;
			char *parameter = malloc( + 1);
			memcpy(parameter, message->paquete.Payload, len);
			parameter[len] = '\0';

//			uint32_t pid;
		//	memcpy(&pid, message->paquete.Payload, len);

			should_int(message->socket) be equal to(DAM_SOCKET);
			should_int(message->paquete.header.tipoMensaje) be equal to(ABRIR_ARCHIVO);
			should_int(message->paquete.header.emisor) be equal to(CPU);
			should_string(parameter) be equal to("/equipos/Banfield.txt");
		}
		end

		/**
		 * Sends to SAFA the pid of the pcb that is getting blocked
		 */
		Mensaje *message_to_safa()
		{
			Mensaje *message = malloc(sizeof(Mensaje)); 
			message->socket = SAFA_SOCKET; 
			message->paquete.header.tipoMensaje = BLOQUEAR_GDT;
			message->paquete.header.emisor = CPU;
			uint32_t pid = 1;
			size_t len = sizeof(uint32_t);
			message->paquete.header.tamPayload = len;
			message->paquete.Payload = malloc(len);
			memmove(message->paquete.Payload, &pid, len);
			return message;
		}

		it("Can make the proper message to SAFA"){
			Mensaje *message = message_to_safa(); 

			size_t len = message->paquete.header.tamPayload;
			uint32_t pid;
			memcpy(&pid, message->paquete.Payload, len);

			should_int(message->socket) be equal to(SAFA_SOCKET);
			should_int(message->paquete.header.tipoMensaje) be equal to(BLOQUEAR_GDT);
			should_int(message->paquete.header.emisor) be equal to(CPU);
			should_int(pid) be equal to(1);
		}
		end

	}
	end
}
