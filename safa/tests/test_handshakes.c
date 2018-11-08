#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <semaphore.h>

        int RETARDO_PLANIF = 2;
        int QUANTUM = 3;
        char* ALGORITMO_PLANIFICACION = "RR";


        typedef enum {CPU, FM9, ELDIEGO, MDJ, SAFA} Emisor;

        typedef enum {
            ESHANDSHAKE, ESSTRING, ESDATOS, SUCCESS, ERROR,									// Mensajes generales
            VALIDAR_ARCHIVO, CREAR_ARCHIVO, OBTENER_DATOS, GUARDAR_DATOS, BORRAR_ARCHIVO, 	// Mensajes MDJ
            DUMMY_SUCCES, DUMMY_FAIL, DTB_SUCCES, DTB_FAIL, DTB_BLOQUEAR, PROCESS_TIMEOUT,  // Mensajes Emisor: CPU, Receptor: SAFA
            ESDTBDUMMY, ESDTB																// Mensajes Emisor: SAFA, Receptor: CPU
            } Tipo;
	
        typedef struct {
            Tipo tipoMensaje;
            int tamPayload;
            Emisor emisor;
        }__attribute__((packed)) Header;

        typedef struct {
            Header header;
            void* Payload;
        }__attribute__((packed)) Paquete;

        typedef enum {
            DTB_NUEVO, DTB_LISTO, DTB_EJECUTANDO, DTB_BLOQUEADO, DTB_FINALIZADO,
            CPU_LIBRE, CPU_OCUPADA
        } Estado;

        typedef struct {
            int socket;
            Estado estado;
        }__attribute__((packed)) t_cpu;


context(test_mensajes_safa) {
    describe("Mensajes de entrada a SAFA") {
        Paquete *handshake_cpu;
        Paquete *handshake_diego;
        // sem_t mutex_handshake_diego;
        // sem_t mutex_handshake_cpu;
        t_list *lista_cpu;
        char mensaje_diego[20];
        int quantum;
        char *algoritmo;

        void mock_handshake_cpu()
        {
            handshake_cpu = malloc(sizeof(Paquete));
            handshake_cpu->header.tipoMensaje = ESHANDSHAKE;
            handshake_cpu->header.tamPayload = 0;
            handshake_cpu->header.emisor = CPU;
        }

        void mock_handshake_diego()
        {
            handshake_diego = malloc(sizeof(Paquete));
            handshake_diego->header.tipoMensaje = ESHANDSHAKE;
            handshake_diego->header.tamPayload = 0;
            handshake_diego->header.emisor = ELDIEGO;
        }

        void leer_config_safa(Paquete * paquete)
		{
			memcpy(&quantum, paquete->Payload + sizeof(u_int32_t), sizeof(u_int32_t));
			int len = 0;
			memcpy(&len, paquete->Payload + sizeof(u_int32_t) * 2, sizeof(u_int32_t));
			algoritmo = malloc(len + 1);
			memcpy(algoritmo, paquete->Payload + sizeof(u_int32_t) * 3, len);
			algoritmo[len] = '\0';
		}

        void cargar_header(Paquete** paquete, int tamanio_payload, Tipo tipo_mensaje, Emisor emisor)
        {
            Header header;
            (*paquete)->header.tamPayload = tamanio_payload;
            (*paquete)->header.tipoMensaje = tipo_mensaje;
            (*paquete)->header.emisor = emisor;
        }

        void *handshake_cpu_serializar(int *tamanio_payload)
        {
            void *payload = malloc(sizeof(RETARDO_PLANIF) + sizeof(QUANTUM));
            int desplazamiento = 0;
            int tamanio = sizeof(u_int32_t);
            memcpy(payload, &RETARDO_PLANIF, tamanio);
            desplazamiento += tamanio;
            memcpy(payload + desplazamiento, &QUANTUM, tamanio);
            desplazamiento += tamanio;

            u_int32_t len_algoritmo = strlen(ALGORITMO_PLANIFICACION);
            payload = realloc(payload, desplazamiento + tamanio);
            memcpy(payload + desplazamiento, &len_algoritmo, tamanio);
            desplazamiento += tamanio;

            payload = realloc(payload, desplazamiento + len_algoritmo);
            memcpy(payload + desplazamiento, ALGORITMO_PLANIFICACION, len_algoritmo);
            desplazamiento += len_algoritmo;

            *tamanio_payload = desplazamiento;
            return payload;
        }

        Paquete *test_enviar_handshake_cpu() {
            int tamanio_payload = 0;
            Paquete* paquete = malloc(sizeof(Paquete));
            paquete->Payload = handshake_cpu_serializar(&tamanio_payload);
            cargar_header(&paquete, tamanio_payload, ESHANDSHAKE, SAFA);
            // EnviarPaquete(socketFD, paquete);
            return paquete;
        }

        Paquete *test_enviar_handshake_diego()
        {
            Paquete* paquete = malloc(sizeof(Paquete));
            cargar_header(&paquete, 0, ESHANDSHAKE, SAFA);

            // EnviarPaquete(socketFD, paquete);
            return paquete;
        }

        void test_manejar_paquetes_CPU(Paquete *paquete, int* socketFD) {
            switch (paquete->header.tipoMensaje) {
                case ESHANDSHAKE: {
                    t_cpu *cpu_nuevo = malloc(sizeof(t_cpu));
                    cpu_nuevo->socket = *socketFD;
                    cpu_nuevo->estado = CPU_LIBRE;
                    list_add(lista_cpu, cpu_nuevo);
                    // log_info(logger, "llegada cpu nuevo en socket: %i", cpu_nuevo->socket);
                    // sem_post(&mutex_handshake_cpu);
                    // enviar_handshake_cpu(*socketFD);
                    break;
                }

                // case DUMMY_SUCCES: {
                //     u_int32_t pid;
                //     memcpy(&pid, paquete->Payload, sizeof(u_int32_t));
                //     notificar_al_PLP(lista_nuevos, &pid);
                //     liberar_cpu(lista_cpu, socketFD);
                //     break;
                // }

                // case DUMMY_FAIL: {
                //     break;
                // }

                // case DTB_BLOQUEAR: {
                //     break;
                // }

                // case PROCESS_TIMEOUT: {
                //     // falta quantum. Chequeo si finaliza o vuelve a listo aca y en succes
                //     break;
                // }

                // case DTB_SUCCES: {
                //     DTB* DTB_succes = DTB_deserializar(paquete->Payload);
                //     int pid = DTB_succes->gdtPID;
                //     // list_remove_by_condition(lista_bloqueados, (void*)coincide_pid(&pid));
                //     // chequear si va a ready o a exit
                //     liberar_cpu(lista_cpu, socketFD);
                //     break;
                // }
            }
        }
        void test_RecibirPaquetes(Paquete paquete, int socketFD)
        {
            switch(paquete.header.emisor) {
                case ELDIEGO:
                {
                    switch (paquete.header.tipoMensaje)
                    {
                        case ESHANDSHAKE:
                        {
                            // sem_post(&mutex_handshake_diego);
                            // log_info(logger, "llegada de el diego");
				    		// test_enviar_handshake_diego(socketFD);
                            strcpy(mensaje_diego, "Llego el diegote");
                            break;
                        }
                    }
                }

                case CPU:
                {
                    test_manejar_paquetes_CPU(&paquete, &socketFD);
                    break;
                }
            }
        }

        void liberar_cpu(void *cpu) {
            free(cpu);
        }

        before
        {
            mock_handshake_cpu();
            mock_handshake_diego();
            // sem_init(&mutex_handshake_diego, 0, 0);
            // sem_init(&mutex_handshake_cpu, 0, 0);
            lista_cpu = list_create();
            strcpy(mensaje_diego, "no llega todavia");
        }
        end

        after
        {
            free(handshake_cpu);
            free(handshake_diego);
            list_destroy_and_destroy_elements(lista_cpu, liberar_cpu);
        }
        end

        it("Entiende recibir handshake de cpu")
        {
            should_int(list_size(lista_cpu)) be equal to (0);
            test_RecibirPaquetes(*handshake_cpu, 1);
            should_int(list_size(lista_cpu)) be equal to (1);
        }
        end
        it("Asigna socket y estado a cpu despues de recibir su handhsake") {
            test_RecibirPaquetes(*handshake_cpu, 1);

            t_cpu *cpu = list_get(lista_cpu, 0);
            should_int(cpu->socket) be equal to (1);
            should_int(cpu->estado) be equal to (CPU_LIBRE);
        }
        end
        it("Recibe handshake de CPU y arma el paquete correcto a enviar") {
            test_RecibirPaquetes(*handshake_cpu, 1);

            Paquete* paquete = test_enviar_handshake_cpu();

            should_int(paquete->header.tipoMensaje) be equal to (ESHANDSHAKE);
            should_int(paquete->header.emisor) be equal to (SAFA);
            should_int(paquete->header.tamPayload) be equal to (14);
            // 14 Bytes = 4 de retardo_planificacion, 4 de quantum, 4 del len_algoritmo y 2 de algoritmo ("rr").

            leer_config_safa(paquete);
            should_int(quantum) be equal to (3);
            should_string(algoritmo) be equal to ("RR");

            free(paquete);

        }
        end
        it("Entiende recibir handshake del diego y arma el paquete correcto a enviar")
        {
            should_string(mensaje_diego) be equal to ("no llega todavia");
            test_RecibirPaquetes(*handshake_diego, 2);
            should_string(mensaje_diego) be equal to ("Llego el diegote");

            Paquete *paquete = test_enviar_handshake_diego();
            should (paquete->header.tamPayload) be equal to (0);
            should (paquete->header.emisor) be equal to (SAFA);
            should (paquete->header.tipoMensaje) be equal to (ESHANDSHAKE);

            free(paquete);
        }
        end
    }
    end
}