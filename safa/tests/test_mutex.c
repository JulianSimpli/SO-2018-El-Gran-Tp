#include <cspecs/cspec.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <stdlib.h>

context(test_mutex_cpu) {

    describe("Comportamiento mutex de estructura cpu"){
        
typedef struct {
    int socket;
    pthread_mutex_t mutex_estado;
}__attribute__((packed)) t_cpu;

t_cpu* cpu1;
t_cpu* cpu2;
t_list* lista_cpu;
int z = 1;

bool coincide_socket(int* socketFD, void* cpu) {
    return ((t_cpu*)cpu)->socket == *socketFD;
}

t_cpu* devuelve_cpu_asociada_a_socket_de_lista(t_list* lista, int* socket) {
    bool compara_cpu(void* cpu) {
        return coincide_socket(socket, cpu);
    }
    return list_find(lista, compara_cpu);
}

int notificar_liberacion_cpu(t_list *lista, int *socketFD) {
    t_cpu* cpu_actual = devuelve_cpu_asociada_a_socket_de_lista(lista, socketFD);
    return pthread_mutex_unlock(&cpu_actual->mutex_estado);
}
bool esta_libre_cpu(t_cpu* cpu) {
    return (int)(cpu->mutex_estado.__align) != 0;
}

bool hay_cpu_libre() {
    return list_any_satisfy(lista_cpu, (void*)esta_libre_cpu);
}

void cpu_mock() {
    cpu1 = malloc(sizeof(t_cpu));
    cpu1->socket = 1;
    pthread_mutex_init(&cpu1->mutex_estado, NULL);
    cpu2 = malloc(sizeof(t_cpu));
    cpu2->socket = 2;
    pthread_mutex_init(&cpu1->mutex_estado, NULL);
}

before {
    lista_cpu = list_create();
    cpu_mock();
} end

after {
    free(cpu1);
    free(cpu2);
} end

it ("El mutex se inicializa en 0") {
    z = cpu1->mutex_estado.__align;
	should_int(z) be equal to (0);
} end

it ("El mutex es 1 al hacer lock") {
    pthread_mutex_lock(&cpu1->mutex_estado);
    z = cpu1->mutex_estado.__align;
	should_int(z) be equal to (1);
} end

it ("El mutex es 0 al hacer unlock") {
    pthread_mutex_unlock(&cpu1->mutex_estado);
    z = cpu1->mutex_estado.__align;
	should_int(z) be equal to (0);
} end

} end
}