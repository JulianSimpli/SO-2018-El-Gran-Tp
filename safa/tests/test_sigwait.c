#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <stdio.h>

// A partir de linea 190 empiezan las funciones a testear.

typedef enum {CPU, FM9, ELDIEGO, MDJ, SAFA} Emisor;

typedef enum {
	ESHANDSHAKE, ESSTRING, ESDATOS, SUCCESS, ERROR,								   // Mensajes generales
	VALIDAR_ARCHIVO, CREAR_ARCHIVO, OBTENER_DATOS, GUARDAR_DATOS, BORRAR_ARCHIVO, 		  // Mensajes MDJ
	DTB_EJECUTO, DTB_BLOQUEAR, PROCESS_TIMEOUT, WAIT, SIGNAL, 		 // Emisor: CPU, Receptor: SAFA
	DUMMY_SUCCES, DUMMY_FAIL, DTB_SUCCES, DTB_FAIL, DTB_FINALIZAR,  			 		// Emisor: Diego, Receptor: SAFA
	ESDTBDUMMY, ESDTB, FIN_EJECUTANDO, ROJADIRECTA, SIGASIGA,								   		   		   // Emisor: SAFA, Receptor: CPU
	FIN_BLOQUEADO 													  		  		  // Emisor: SAFA, Receptor: Diego												
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

typedef struct DTB_info {
	u_int32_t gdtPID;
	Estado estado;
	int socket_cpu;
	clock_t *tiempo_ini;
	clock_t *tiempo_fin;
	float tiempo_respuesta;
	bool kill;
	t_list *recursos;
} DTB_info;

typedef struct {
	char *id;
	u_int32_t semaforo;
	t_list *pid_bloqueados;
}__attribute__((packed)) t_recurso;

context(wait_signal_recursos)
{
    describe("Mensajes de cpu wait y signal")
    {
        t_list *lista_recursos_global;
        t_list *lista_listos;
        t_list *lista_ejecutando;
        t_list *lista_bloqueados;
        t_list *lista_info_dtb;

        void cargar_header(Paquete** paquete, int tamanio_payload, Tipo tipo_mensaje, Emisor emisor)
        {
            Header header;
            (*paquete)->header.tamPayload = tamanio_payload;
            (*paquete)->header.tipoMensaje = tipo_mensaje;
            (*paquete)->header.emisor = emisor;
        }

        bool info_coincide_pid(u_int32_t pid, void *info_dtb)
        {
            return ((DTB_info *)info_dtb)->gdtPID == pid;
        }

        DTB_info *info_asociada_a_pid(u_int32_t pid)   //No hace lo mismo que la de arriba? COMPLETAR
        {
            bool compara_con_info(void *info_dtb)
            {
                return info_coincide_pid(pid, info_dtb);
            }
            return list_find(lista_info_dtb, compara_con_info);
        }

        bool dtb_coincide_pid(void* dtb, u_int32_t pid, u_int32_t flag)
        {
            return ((DTB *)dtb)->gdtPID == pid && ((DTB *)dtb)->flagInicializacion == flag;
        }
       
        DTB *dtb_remueve(t_list* lista, u_int32_t pid, u_int32_t flag)
        {
            bool compara_con_dtb(void* dtb) {
                return dtb_coincide_pid(dtb, pid, flag);
            }
            return list_remove_by_condition(lista, compara_con_dtb);
        }

        DTB *dtb_encuentra(t_list* lista, u_int32_t pid, u_int32_t flag)
        {
            bool compara_con_dtb(void* dtb) {
                return dtb_coincide_pid(dtb, pid, flag);
            }
            return list_find(lista, compara_con_dtb);
        }

        DTB_info* info_dtb_actualizar(Estado estado, int socket, DTB_info *info_dtb)
        {
            info_dtb->socket_cpu = socket;
            info_dtb->estado = estado;
            return info_dtb;
        }

        DTB *dtb_actualizar(DTB *dtb, t_list *source, t_list *dest, u_int32_t pc, Estado estado, int socket)
        {
            if(dtb_encuentra(source, dtb->gdtPID, dtb->flagInicializacion) != NULL)
                dtb_remueve(source, dtb->gdtPID, dtb->flagInicializacion);

            list_add(dest, dtb);
            dtb->PC = pc;

            DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
            info_dtb_actualizar(estado, socket, info_dtb);
            // switch(estado)
            // {
            //     case DTB_FINALIZADO: procesos_finalizados++;
            //     case DTB_LISTO: procesos_en_memoria++;
            // }

            return dtb;
        }

                void *string_serializar(char *string, int *desplazamiento)
        {
            u_int32_t len_string = strlen(string);
            void *serializado = malloc(sizeof(len_string) + len_string);

            memcpy(serializado + *desplazamiento, &len_string, sizeof(len_string));
            *desplazamiento += sizeof(len_string);
            memcpy(serializado + *desplazamiento, string, len_string);
            *desplazamiento += len_string;

            return serializado;
        }

        char *string_deserializar(void *data, int *desplazamiento)
        {         
            u_int32_t len_string = 0;
            memcpy(&len_string, data + *desplazamiento, sizeof(len_string));
            *desplazamiento += sizeof(len_string);
            
            char *string = malloc(len_string + 1);
            memcpy(string, data + *desplazamiento, len_string);
            *(string+len_string) = '\0';
            *desplazamiento = *desplazamiento + len_string + 1;

            return string;
        }

        t_recurso *recurso_crear(char *id_recurso, int valor_inicial)
        {
            t_recurso *recurso = malloc(sizeof(t_recurso));
            recurso->id = malloc(strlen(id_recurso) + 1);
            strcpy(recurso->id, id_recurso);
            recurso->semaforo = valor_inicial;
            recurso->pid_bloqueados = list_create();

            list_add(lista_recursos_global, recurso);

            return recurso;
        }

        void recurso_liberar(t_recurso *recurso)
        {
            free(recurso->id);
            list_destroy_and_destroy_elements(recurso->pid_bloqueados, free);
            free(recurso);
        }

        bool coincide_id(void *recurso, char *id)
        {
            return !strcmp(((t_recurso *)recurso)->id, id);
        }

        t_recurso *recurso_encontrar(char* id_recurso)
        {
            bool comparar_id(void *recurso)
            {
                return coincide_id(recurso, id_recurso);
            }

            return list_find(lista_recursos_global, comparar_id);
        }

// A partir de aca empiezan las funciones a testear.

        void *cargar_pc_y_pid(u_int32_t pid, u_int32_t pc, int *tamanio_payload)
        {
            void *payload = malloc(sizeof(u_int32_t) * 2);

            memcpy(payload, &pid, sizeof(u_int32_t));
            *tamanio_payload += sizeof(u_int32_t);
            memcpy(payload + *tamanio_payload, &pc, sizeof(u_int32_t));
            *tamanio_payload += sizeof(u_int32_t);

            return payload;
        }

        void seguir_ejecutando(u_int32_t pid, u_int32_t pc, int socket)
        {
            Paquete *paquete = malloc(sizeof(Paquete));
            int tamanio_payload = 0;
            paquete->Payload = cargar_pc_y_pid(pid, pc, &tamanio_payload);
            cargar_header(&paquete, tamanio_payload, SIGASIGA, SAFA);
            // Comentado solo para tests
            // EnviarPaquete(socket, paquete);
            // free(paquete);
        }

        void recurso_asignar_a_pid(t_recurso *recurso, u_int32_t pid)
        {
            recurso->semaforo--;
            DTB_info *info_dtb = info_asociada_a_pid(pid);
            list_add(info_dtb->recursos, recurso->id);
        }

        DTB *dtb_signal(t_recurso *recurso, int socket)
        {
            int pid = list_remove(recurso->pid_bloqueados, 0);
            DTB *dtb = dtb_encuentra(lista_bloqueados, pid, 1);

            dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC, DTB_LISTO, socket);
            recurso_asignar_a_pid(recurso, pid);

            return dtb;
        }

        DTB *dtb_bloquear(u_int32_t pid, u_int32_t pc, int socket)
        {
            Paquete *paquete = malloc(sizeof(Paquete));
            int tamanio_payload = 0;
            paquete->Payload = cargar_pc_y_pid(pid, pc, &tamanio_payload);
            cargar_header(&paquete, tamanio_payload, ROJADIRECTA, SAFA);
            // Comentado solo para test
            // EnviarPaquete(socket, paquete);
            // free(paquete);

            DTB *dtb = dtb_encuentra(lista_ejecutando, pid, 1);
            dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, pc, DTB_BLOQUEADO, socket);

            return dtb;
        }

        void wait(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
        {
            if(recurso->semaforo-- < 0)
            {
                list_add(recurso->pid_bloqueados, pid);
                dtb_bloquear(pid, pc, socket);
            }
            else
            {
                recurso_asignar_a_pid(recurso, pid);
                seguir_ejecutando(pid, pc, socket);
            }
        }
        
        void signal(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
        {
            if(recurso->semaforo++ <= 0)
            {
                dtb_signal(recurso, socket);
                seguir_ejecutando(pid, pc, socket);
            }
            else seguir_ejecutando(pid, pc, socket);
        }

        t_recurso *recibir_recurso(void *payload, int *pid, int *pc)
        {
            int desplazamiento = 0;
            char *id_recurso = string_deserializar(payload, &desplazamiento);

            memcpy(pid, payload + desplazamiento, sizeof(u_int32_t));
            desplazamiento += sizeof(u_int32_t);
            memcpy(pc, payload + desplazamiento, sizeof(u_int32_t));
            desplazamiento += sizeof(u_int32_t);
            free(payload);

            t_recurso *recurso = recurso_encontrar(id_recurso);
            if (recurso == NULL)
                recurso = recurso_crear(id_recurso, 1);

            return recurso;
        }

        void test_manejar_paquetes_CPU(Paquete *paquete, int socketFD)
        {
            switch (paquete->header.tipoMensaje)
            {
                case WAIT:
                {
                    u_int32_t pid;
                    u_int32_t pc;
                    t_recurso *recurso = recibir_recurso(paquete->Payload, &pid, &pc);
                    wait(recurso, pid, pc, socketFD);
                }
                case SIGNAL:
                {
                    u_int32_t pid;
                    u_int32_t pc;
                    t_recurso *recurso = recibir_recurso(paquete->Payload, &pid, &pc);
                    signal(recurso, pid, pc, socketFD);
                }
            }
        }
        
        before
        {
            lista_recursos_global = list_create();
            lista_listos = list_create();
            lista_ejecutando = list_create();
            lista_bloqueados = list_create();
            lista_info_dtb = list_create();
        }
        end
        
        after
        {
            list_destroy(lista_ejecutando);
        }
        end
        it("Maneja llegada de wait con un recurso existente")
        {

        }
        end

        it("Bloquea DTB tras wait con sem en 0")
        {
            should_bool(false) be truthy;
        }
        end

        it("NO bloquea DTB tras wait con sem en 1")
        {
            should_bool(false) be truthy;
        }
        end

        it("Desbloquea DTB tras signal con sem en -1")
        {
            should_bool(false) be truthy;
        }
        end
    }
    end
}