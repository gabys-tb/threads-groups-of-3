#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>
#include <unistd.h>
#include <time.h>

struct RoomData {
    int id;
    int time;

    RoomData(int id, int time) : id(id), time(time) {}
};

struct ThreadData {
    pthread_t thread;
    int id;
    int init_time;
    int n_rooms;
    std::queue<RoomData> queue;

    ThreadData(int id, int init_time, int n_rooms, std::queue<RoomData>&& queue)
        : id(id), init_time(init_time), n_rooms(n_rooms), queue(std::move(queue)) {}
};

struct Room {
    pthread_mutex_t mtx;
    pthread_cond_t wait_to_enter;
    pthread_cond_t wait_trio;
    
    int occupancy = 0; // Contador de ocupação da sala
    int waiting_threads = 0; // Contador de threads esperando para formar trio
    int completed_trio_threads = 0; // Contador de threads que completaram o tempo na sala
    int formed_trio = 0; // Contador de trio de threads formado 

    // Construtor para inicializar o mutex e a variável de condição
    Room() {
        pthread_mutex_init(&mtx, NULL);
        pthread_cond_init(&wait_trio, NULL);
        pthread_cond_init(&wait_to_enter, NULL);
    }

    // Destrutor para destruir o mutex e a variável de condição
    ~Room() {
        pthread_mutex_destroy(&mtx);
        pthread_cond_destroy(&wait_to_enter);
        pthread_cond_destroy(&wait_trio);
    }
};

// Variáveis globais
std::vector<Room> rooms;
std::vector<ThreadData> threads;


void passa_tempo(int tid, int sala, int decimos) {
    struct timespec zzz, agora;
    static struct timespec inicio = {0, 0};
    int tstamp;

    if ((inicio.tv_sec == 0) && (inicio.tv_nsec == 0)) {
        clock_gettime(CLOCK_REALTIME, &inicio);
    }

    zzz.tv_sec = decimos / 10;
    zzz.tv_nsec = (decimos % 10) * 100L * 1000000L;

    if (sala == 0) {
        nanosleep(&zzz, NULL);
        return;
    }

    clock_gettime(CLOCK_REALTIME, &agora);
    tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) -
             (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

    printf("%3d [ %2d @%2d z%4d\n", tstamp, tid, sala, decimos);

    nanosleep(&zzz, NULL);

    clock_gettime(CLOCK_REALTIME, &agora);
    tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) -
             (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

    printf("%3d ) %2d @%2d\n", tstamp, tid, sala);
}

// Função para entrar na sala
void enter(int tid, int id) {
    // Bloqueia o mutex da sala
    pthread_mutex_lock(&rooms[id].mtx);
    
    // Faz threads que tentam entrar esperarem
    while (rooms[id].formed_trio == 3) {
        pthread_cond_wait(&rooms[id].wait_to_enter, &rooms[id].mtx);
    }
    rooms[id].formed_trio++;
    
    pthread_mutex_unlock(&rooms[id].mtx);  // Desbloqueia o mutex
}

// Função para sair da sala
void leave(int tid, int id) {
    // Bloqueia o mutex da sala
    pthread_mutex_lock(&rooms[id].mtx);
    rooms[id].completed_trio_threads++;

    // Se todas as três threads do trio saíram, libera a sala para o próximo trio
    if (rooms[id].completed_trio_threads == 3) {
        rooms[id].formed_trio = 0;  // Libera a ocupação da sala
        rooms[id].completed_trio_threads = 0;  // Reseta o contador de threads concluídas
       
        pthread_cond_broadcast(&rooms[id].wait_to_enter);  // Notifica as threads que estão esperando
    }
    pthread_mutex_unlock(&rooms[id].mtx);  // Desbloqueia o mutex
    
}

void* thread_func(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    // Aguarda o tempo inicial antes de iniciar o trajeto
    passa_tempo(data->id, 0, data->init_time);

    // Executa o trajeto conforme a lista de salas
    while (!data->queue.empty()) {
        RoomData room = data->queue.front();
        data->queue.pop();

        // Entra na sala em trio
        enter(data->id, room.id);

        pthread_mutex_lock(&rooms[room.id].mtx);
        
        // Aguarda até que todas threads estejam prontas para entrar
        if (rooms[room.id].formed_trio == 3) {
            // Quando a última thread chega, faz o broadcast
            pthread_cond_broadcast(&rooms[room.id].wait_trio);
        } else {
            // As outras threads ficam esperando o broadcast
            pthread_cond_wait(&rooms[room.id].wait_trio, &rooms[room.id].mtx);     
        }
        
        pthread_mutex_unlock(&rooms[room.id].mtx);
       
        // Passa o tempo necessário na sala
        passa_tempo(data->id, room.id, room.time);

        // Sai da sala
        leave(data->id, room.id);
    }
    return NULL;
}

int main() {
    int s, t;
    std::cin >> s >> t;

    rooms = std::vector<Room>(s+1);  // Inicializa `rooms` com `s` salas
    threads.reserve(t);

    for (int i = 0; i < t; i++) {
        int id, init_time, n_rooms;
        std::cin >> id >> init_time >> n_rooms;

        std::queue<RoomData> queue;
        for (int j = 0; j < n_rooms; j++) {
            int id, time;
            std::cin >> id >> time;
            queue.push(RoomData(id, time)); 
        }

        threads.emplace_back(id, init_time, n_rooms, std::move(queue));
    }

    // Cria as threads
    for (auto& thread_data : threads) {
        pthread_create(&thread_data.thread, NULL, thread_func, &thread_data);
    }

    // Espera todas as threads terminarem
    for (auto& thread_data : threads) {
        pthread_join(thread_data.thread, NULL);
    }

    return 0;
}
