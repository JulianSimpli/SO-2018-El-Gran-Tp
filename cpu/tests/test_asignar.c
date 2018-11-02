#include <commons/log.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <commons/string.h>
#include <string.h>
#include <cspecs/cspec.h>
#include "../../Bibliotecas/sockets.h"

context(test_primitives)
{

	describe("CPU understand the assign primitive")
	{

		char *message = "asignar /equipos/Racing.txt 9 GustavoBou";
		int DAM_SOCKET = 10;
		int SAFA_SOCKET = 11;

		/**
		 * Errores Esperables
		 * 20001: El archivo no se encuentra abierto.
		 * 20002: Fallo de segmento/memoria.
		 * 20003: Espacio insuficiente en FM9.
		 */

		/**
		 * This will depend if pcb has a list or a char**
		 */
		t_list *get_open_files(void *pcb)
		{
			t_list *files = list_create();
			list_add(files, (void *)"/equipos/Racing.txt");
			return files;
		}

		bool find_file(t_list * files, char *path)
		{
			bool match_path_name(void *element)
			{
				return strcmp(path, (char *)element) == 0;
			}

			return list_any_satisfy(files, match_path_name);
		}

		/**
		 * Verificará que el archivo que se desea modificar se encuentre abierto por el G.DT.
		 * Para esto se utilizaran las estructuras administrativas del DTB.
		 * En caso que no se encuentre abierto, se abortará el G.DT informando el error a S-AFA.
		 */
		bool is_file_already_open(char *path)
		{
			t_list *files = get_open_files((void *)NULL);

			return find_file(files, path);
		}

		/**
		 * Return true if it can continue to the next primitive
		 * Returns false otherwise
		 */
		bool handle_asignar(char **words)
		{
			char *parameter = words[1];

			if (!is_file_already_open(parameter))
			{
				//send_message(, SAFA);
				return false;
			}

			//send_message(, FM9);
			return true;
		}

		it("Can parse command from message")
		{
			char **words = string_split(message, " ");
			char *primitive = words[0];
			should_string(primitive) be equal to("abrir");
		}
		end

		it("Can parse parameters from message")
		{
			char **words = string_split(message, " ");
			char *parameter = words[1];
			int lines = atoi(words[2]);
			char *player = words[3];
			should_string(parameter) be equal to("/equipos/Racing.txt");
			should_int(lines) be equal to(9);
			should_string(player) be equal to("GustavoBou");
		}
		end

		it("Can find a matching path on a list")
		{
			t_list *files = list_create();
			list_add(files, (void *)"test");
			bool match = find_file(files, "test");
			should_bool(match) be truthy;
		}
		end

		it("Can check if a file is already open in a pcb")
		{
			char **words = string_split(message, " ");
			char *parameter = words[1];
			bool status = is_file_already_open(parameter);

			should_bool(status) be truthy;
		}
		end

		it("The file is not open and can not continue")
		{
			char *message = "asignar /equipos/Independiente.txt 2 CosmeFulanito";
			char **words = string_split(message, " ");

			bool status = handle_asignar(words);

			should_bool(status) be falsey;
		}
		end

		it("Send data to fm9 and goes to the next primitive")
		{
			char **words = string_split(message, " ");
			bool status = handle_asignar(words);
			should_bool(status) be truthy;
		}
		end

		/**
		 * Se enviará a FM9 los datos necesarios para actualizar el valor en memoria.
		 */
		Mensaje *mensaje_a_fm9()
		{
			Mensaje *mensaje = malloc(sizeof(Mensaje));
			mensaje->socket = DAM_SOCKET;
			mensaje->paquete.header.tipoMensaje = 1;
			mensaje->paquete.header.emisor = CPU;
			char *parameter = "/equipos/Banfield.txt";
			size_t len = strlen(parameter);
			mensaje->paquete.header.tamPayload = len;
			mensaje->paquete.Payload = malloc(len);
			memcpy(mensaje->paquete.Payload, parameter, len);
			return mensaje;
		}

		it("Can make the proper message to FM9")
		{
			Mensaje *message = mensaje_a_fm9();

			size_t len = message->paquete.header.tamPayload;
			char *parameter = malloc(len + 1);
			memcpy(parameter, message->paquete.Payload, len);
			parameter[len] = '\0';

			should_int(message->socket) be equal to(DAM_SOCKET);
			should_int(message->paquete.header.tipoMensaje) be equal to(ABRIR_ARCHIVO);
			should_int(message->paquete.header.emisor) be equal to(CPU);
			should_string(parameter) be equal to("/equipos/Banfield.txt");
		}
		end

		/**
		 * Sends to SAFA the pid of the pcb that is getting blocked
		 * and the proper error code
		 */
		Mensaje *message_to_safa()
		{
			Mensaje *message = malloc(sizeof(Mensaje));
			message->socket = SAFA_SOCKET;
			message->paquete.header.tipoMensaje = BLOQUEAR_GDT;
			message->paquete.header.emisor = CPU;

			/*
			__intptr_t pid = 1;
			size_t len = sizeof(__intptr_t);
			message->paquete.header.tamPayload = len;
			message->paquete.Payload = malloc(len);
			memcpy(message->paquete.Payload, (void*)pid, len);
			*/

			return message;
		}

		it("Can make the proper message to SAFA")
		{
			Mensaje *message = message_to_safa();

			//size_t len = message->paquete.header.tamPayload;
			//should_int((__intptr_t)message->paquete.Payload) be equal to(1);
			//uint32_t pid;
			//memcpy(&pid, message->paquete.Payload, len);

			should_int(message->socket) be equal to(SAFA_SOCKET);
			should_int(message->paquete.header.tipoMensaje) be equal to(ABRIR_ARCHIVO);
			should_int(message->paquete.header.emisor) be equal to(CPU);
			//should_int(pid) be equal to(0);
		}
		end
	}
	end
}
