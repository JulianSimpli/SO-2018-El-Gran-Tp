#include <cspecs/cspec.h>
#include "../../Bibliotecas/dtb.h"
#include <stdio.h>

#define GDT 1
#define DUMMY 0

// A partir de 225 empiezan funciones de recursos.
// A partir de linea 290 empiezan las funciones a testear.

u_int32_t pid = 0;
u_int32_t pc = 0;

u_int32_t numero_pid = 0;

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
	int semaforo;
	t_list *pid_bloqueados;
}__attribute__((packed)) t_recurso;

context(wait_signal_recursos)
{
    describe("Mensajes de cpu wait y signal")
    {
        t_list *lista_recursos_global;
        t_list *lista_nuevos;
        t_list *lista_listos;
        t_list *lista_ejecutando;
        t_list *lista_bloqueados;
        t_list *lista_info_dtb;

        t_recurso *recurso1;
        Paquete *paquete_wait;
        Paquete *paquete2_wait;

        DTB *dtb;

        Header cargar_header(int tamanio_payload, Tipo tipo_mensaje, Emisor emisor)
        {   
            Header header;
            header.tamPayload = tamanio_payload;
            header.tipoMensaje = tipo_mensaje;
            header.emisor = emisor;

            return header;
        }

        ArchivoAbierto *_DTB_crear_archivo(int cant_lineas, char *path)
        {
            ArchivoAbierto *archivo = malloc(sizeof(ArchivoAbierto));
            archivo->cantLineas = cant_lineas;
            archivo->path = malloc(strlen(path)+1);
            strcpy(archivo->path, path);
            return archivo;
        }

        void liberar_archivo_abierto(void *archivo)
        {
            free(((ArchivoAbierto *)archivo)->path);
            free(archivo);
        }

        void DTB_agregar_archivo(DTB *dtb, int cant_lineas, char *path)
        {
            ArchivoAbierto *archivo = _DTB_crear_archivo(cant_lineas, path);
            list_add(dtb->archivosAbiertos, archivo);
        }
        DTB_info *info_dtb_crear(u_int32_t pid)
        {
            DTB_info *info_dtb = malloc(sizeof(DTB_info));
            info_dtb->gdtPID = pid;
            info_dtb->estado = DTB_NUEVO;
            info_dtb->socket_cpu = 0;
            info_dtb->tiempo_respuesta = 0;
            info_dtb->kill = false;
            info_dtb->recursos = list_create();
            //*info_dtb->tiempo_ini
            //*info_dtb->tiempo_fin
            list_add(lista_info_dtb, info_dtb);

            return info_dtb;
        }

        DTB *dtb_crear(u_int32_t pid, char* path, int flag_inicializacion)
        {
            DTB *dtb_nuevo = malloc(sizeof(DTB));
            dtb_nuevo->flagInicializacion = flag_inicializacion;
            dtb_nuevo->PC = 0;
            dtb_nuevo->archivosAbiertos = list_create();
            DTB_agregar_archivo(dtb_nuevo, 0, path);

            switch(flag_inicializacion)
            {
                case DUMMY:
                {
                    dtb_nuevo->gdtPID = pid;
                    list_add(lista_listos, dtb_nuevo);
                    break;
                }
                case GDT:
                {
                    dtb_nuevo->gdtPID = ++numero_pid;
                    info_dtb_crear(dtb_nuevo->gdtPID);
                    list_add(lista_nuevos, dtb_nuevo);
                    break;
                }
                default:
                    printf("flag_inicializacion invalida");
            }

            return dtb_nuevo;
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

        void info_liberar(void *dtb)
        {
            u_int32_t pid = ((DTB *)dtb)->gdtPID;
            bool coincide_info(void *info_dtb)
            {
                return info_coincide_pid(pid, info_dtb);
            }
            list_remove_and_destroy_by_condition(lista_info_dtb, coincide_info, free);
        }

        void dtb_liberar(void *dtb)
        {
            if (((DTB *)dtb)->flagInicializacion == GDT)
                info_liberar(dtb);
            list_clean_and_destroy_elements(((DTB *)dtb)->archivosAbiertos, liberar_archivo_abierto);
            free(dtb);
        }

// Empiezan funciones de recursos

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
            *desplazamiento = *desplazamiento + len_string;

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
            list_destroy(recurso->pid_bloqueados);
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

        void *serializar_pid_y_pc(u_int32_t pid, u_int32_t pc, int *tam_pid_y_pc)
        {
            void *payload = malloc(sizeof(u_int32_t) * 2);

            memcpy(payload + *tam_pid_y_pc, &pid, sizeof(u_int32_t));
            *tam_pid_y_pc += sizeof(u_int32_t);
            memcpy(payload + *tam_pid_y_pc, &pc, sizeof(u_int32_t));
            *tam_pid_y_pc += sizeof(u_int32_t);

            return payload;
        }

        void seguir_ejecutando(u_int32_t pid, u_int32_t pc, int socket)
        {
            Paquete *paquete = malloc(sizeof(Paquete));
            int tamanio_payload = 0;
            paquete->Payload = serializar_pid_y_pc(pid, pc, &tamanio_payload);
            paquete->header = cargar_header(tamanio_payload, SIGASIGA, SAFA);
            // Comentado solo para tests
            // EnviarPaquete(socket, paquete);
            free(paquete->Payload);
            free(paquete);
        }

        void recurso_asignar_a_pid(t_recurso *recurso, u_int32_t pid)
        {
            DTB_info *info_dtb = info_asociada_a_pid(pid);
            list_add(info_dtb->recursos, recurso->id);
        }

        DTB *dtb_signal(t_recurso *recurso, int socket)
        {
            u_int32_t pid = list_remove(recurso->pid_bloqueados, 0);
            DTB *dtb = dtb_encuentra(lista_bloqueados, pid, 1);

            dtb_actualizar(dtb, lista_bloqueados, lista_listos, dtb->PC, DTB_LISTO, socket);
            recurso->semaforo--;
            recurso_asignar_a_pid(recurso, pid);

            return dtb;
        }

        DTB *dtb_bloquear(u_int32_t pid, u_int32_t pc, int socket)
        {
            Paquete *paquete = malloc(sizeof(Paquete));
            int tamanio_payload = 0;
            paquete->Payload = serializar_pid_y_pc(pid, pc, &tamanio_payload);
            paquete->header = cargar_header(tamanio_payload, ROJADIRECTA, SAFA);
            // Comentado solo para test
            // EnviarPaquete(socket, paquete);
            // free(paquete);

            DTB *dtb = dtb_encuentra(lista_ejecutando, pid, 1);
            dtb_actualizar(dtb, lista_ejecutando, lista_bloqueados, pc, DTB_BLOQUEADO, socket);

            return dtb;
        }

        void wait(t_recurso *recurso, u_int32_t pid, u_int32_t pc, int socket)
        {
            recurso->semaforo--;
            if(recurso->semaforo < 0)
            {
                list_add(recurso->pid_bloqueados, &pid);
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
            recurso->semaforo++;
            if(recurso->semaforo <= 0)
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

            t_recurso *recurso = recurso_encontrar(id_recurso);
            if (recurso == NULL) recurso = recurso_crear(id_recurso, 1);

            return recurso;
        }

        void test_manejar_paquetes_CPU(Paquete *paquete, int socketFD)
        {
            switch (paquete->header.tipoMensaje)
            {
                case WAIT:
                {
                    // u_int32_t pid; Comentadas porque son variables globales en tests, para poder mostrarlos.
                    // u_int32_t pc;
                    t_recurso *recurso = recibir_recurso(paquete->Payload, &pid, &pc);
                    //dtb asociado a pid, y a wait le paso como parametro dtb->gdtPID porque tiene que ser dato persistido en memoria
                    // si no, se va a borrar cuando salga del bloque
                    wait(recurso, pid, pc, socketFD);
                    break;
                }
                case SIGNAL:
                {
                    // u_int32_t pid;
                    // u_int32_t pc;
                    t_recurso *recurso = recibir_recurso(paquete->Payload, &pid, &pc);
                    //signal(&recurso, pid, pc, socketFD);
                    break;
                }
            }
        }

        Paquete *paquete_wait_mock()
        {
            u_int32_t pc = 10;
            u_int32_t pid = dtb->gdtPID;
            char *id_recurso = "rec_nuevo";

            paquete_wait = malloc(sizeof(Paquete));

            int tam_string = 0;
            paquete_wait->Payload = string_serializar(id_recurso, &tam_string);

            int tam_pid_y_pc = 0;
            void *pc_y_pid = serializar_pid_y_pc(pid, pc, &tam_pid_y_pc);
            int tam_total = tam_string + tam_pid_y_pc;

            paquete_wait->Payload = realloc(paquete_wait->Payload, tam_total);
            memcpy(paquete_wait->Payload + tam_string, pc_y_pid, tam_pid_y_pc); 
            free(pc_y_pid);

            paquete_wait->header = cargar_header(tam_total, WAIT, CPU);

            //EnviarPaquete();
            //free(paquete);
        }

        Paquete *paquete2_wait_mock()
        {
            u_int32_t pc = 15;
            u_int32_t pid = dtb->gdtPID;
            char *id_recurso = "rec_existente";

            paquete2_wait = malloc(sizeof(Paquete));

            int tam_string = 0;
            paquete2_wait->Payload = string_serializar(id_recurso, &tam_string);

            int tam_pid_y_pc = 0;
            void *pc_y_pid = serializar_pid_y_pc(pid, pc, &tam_pid_y_pc);
            int tam_total = tam_string + tam_pid_y_pc;

            paquete2_wait->Payload = realloc(paquete2_wait->Payload, tam_total);
            memcpy(paquete2_wait->Payload + tam_string, pc_y_pid, tam_pid_y_pc); 
            free(pc_y_pid);

            paquete2_wait->header = cargar_header(tam_total, WAIT, CPU);

            //EnviarPaquete();
            //free(paquete);
        }

        Paquete *paquete_signal_mock()
        {
            u_int32_t pc = 15;
            u_int32_t pid = dtb->gdtPID;
            char *id_recurso = "rec_existente";

            paquete2_wait = malloc(sizeof(Paquete));

            int tam_string = 0;
            paquete2_wait->Payload = string_serializar(id_recurso, &tam_string);

            int tam_pid_y_pc = 0;
            void *pc_y_pid = serializar_pid_y_pc(pid, pc, &tam_pid_y_pc);
            int tam_total = tam_string + tam_pid_y_pc;

            paquete2_wait->Payload = realloc(paquete2_wait->Payload, tam_total);
            memcpy(paquete2_wait->Payload + tam_string, pc_y_pid, tam_pid_y_pc); 
            free(pc_y_pid);

            paquete2_wait->header = cargar_header(tam_total, WAIT, CPU);

            //EnviarPaquete();
            //free(paquete);
        }
        
        void dtb_mock()
        {
            dtb = dtb_crear(1, "path/a.txt", GDT); // GDT es la flag
            dtb_actualizar(dtb, lista_nuevos, lista_ejecutando, dtb->PC, DTB_EJECUTANDO, 1); // 1 es el socket
        }

        before
        {
            lista_recursos_global = list_create();

            lista_nuevos = list_create();
            lista_listos = list_create();
            lista_ejecutando = list_create();
            lista_bloqueados = list_create();

            lista_info_dtb = list_create();

            dtb_mock();
            recurso1 = recurso_crear("rec_existente", 1);
            paquete_wait_mock();
            paquete2_wait_mock();
        }
        end
        
        after
        {
            list_destroy(lista_nuevos);
            list_destroy_and_destroy_elements(lista_ejecutando, dtb_liberar);
            list_destroy_and_destroy_elements(lista_listos, dtb_liberar);
            list_destroy_and_destroy_elements(lista_bloqueados, dtb_liberar);

            list_destroy_and_destroy_elements(lista_recursos_global, recurso_liberar);
            numero_pid = 0;
        }
        end

        it("recurso1, dtb con campos correctos")
        {
            should_int(recurso1->semaforo) be equal to (1);
            should_string(recurso1->id) be equal to ("rec_existente");
            should_int(list_size(recurso1->pid_bloqueados)) be equal to (0);

            should_int(dtb->gdtPID) be equal to (1);
        }
        end

        it("Crea/recibe bien el paquete salida/llegada de WAIT de recurso nuevo. Deja semaforo en 0")
        {
            should_int(list_size(lista_recursos_global)) be equal to (1);
            test_manejar_paquetes_CPU(paquete_wait, 1);

            should_int(list_size(lista_recursos_global)) be equal to (2);
            t_recurso *rec_nuevo = list_get(lista_recursos_global, 1);
            should_string(rec_nuevo->id) be equal to ("rec_nuevo");
            should_int(rec_nuevo->semaforo) be equal to (0);
            should_int(list_size(rec_nuevo->pid_bloqueados)) be equal to (0);

            should_int(pc) be equal to (10);
            should_int(pid) be equal to (dtb->gdtPID);

            free(paquete_wait);
        }
        end

        it("Crea/recibe bien el paquete salida/llegada de WAIT de recurso existente. Deja semaforo en 0")
        {
            should_int(list_size(lista_recursos_global)) be equal to (1);
            test_manejar_paquetes_CPU(paquete2_wait, 1);

            should_int(list_size(lista_recursos_global)) be equal to (1);
            t_recurso *rec_viejo = list_get(lista_recursos_global, 0);
            should_string(rec_viejo->id) be equal to ("rec_existente");
            should_int(rec_viejo->semaforo) be equal to (0);
            should_int(list_size(rec_viejo->pid_bloqueados)) be equal to (0);

            should_int(pc) be equal to (15);
            should_int(pid) be equal to (dtb->gdtPID);

            free(paquete2_wait);
        }
        end

        it("WAIT asigna recurso a pid, si semaforo es >0")
        {
            should_int(recurso1->semaforo) be equal to (1);
            should_string(recurso1->id) be equal to ("rec_existente");
            should_int(list_size(recurso1->pid_bloqueados)) be equal to (0);

            DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
            should_int(list_size(info_dtb->recursos)) be equal to (0);
            wait(recurso1, dtb->gdtPID, 10, 1);
            should_int(list_size(info_dtb->recursos)) be equal to (1);
            char *id_rec = list_get(info_dtb->recursos, 0);
            should_string(id_rec) be equal to ("rec_existente");   
        }
        end

        it("WAIT bloquea a pid, si semaforo es <=0")
        {
            should_int(list_size(recurso1->pid_bloqueados)) be equal to (0);
            wait(recurso1, dtb->gdtPID, 10, 1);
            should_int(list_size(recurso1->pid_bloqueados)) be equal to (0);
            wait(recurso1, dtb->gdtPID, 11, 1);
            should_int(list_size(recurso1->pid_bloqueados)) be equal to (1);
            
            DTB_info *info_dtb = info_asociada_a_pid(dtb->gdtPID);
            should_int(info_dtb->estado) be equal to (DTB_BLOQUEADO);
            should_int(list_size(lista_bloqueados)) be equal to (1);
        }
        end

        it("Crea/recibe bien el paquete de salida/llegada de SIGNAL")
        {
            should_bool(false) be truthy;
        }
        end
    }
    end
}