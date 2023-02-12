#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <time.h>

// definindo o tamanho do buffer;
#define N 10

// definindo chaves e variáveis de ID para memória compartilhada;
key_t k1 = 1234, k2 = 2345, k3 = 3456, k4 = 4567;
int shmid1, shmid2, shmid3, shmid4;

// definindo os semáforos e o buffer;
typedef int semaphore;
semaphore *mutex;
semaphore *empty;
semaphore *full;
int *buffer;

// função wait() decrementa o semáforo caso possível;
// caso não, coloca o processo para dormir;
void wait(semaphore *s){
    while (s <= 0){
        sleep(3);
    }
    (*s)--;
}

// função signal() acorda o processo dormindo incrementando o semáforo;
void signal(semaphore *s){
    (*s)++;
}

void insert(int item){
    // referenciando o espaço de memória compartilhada;
    mutex = shmat(shmid1, NULL, 0);
    empty = shmat(shmid2, NULL, 0);
    full = shmat(shmid3, NULL, 0);
    buffer = shmat(shmid4, NULL, 0);

    // verificando se o buffer está cheio;
    // caso esteja, o processo é posto para dormir;
    if (*mutex == N){
        wait(full);
    }

    int index = 0;

    while (index < N){

        if (buffer[index] == 0){
            buffer[index] = item;
            break;
        }

        index++;
    }

    // incrementando váriavel de controle de acesso a região crítica;
    (*mutex)++;

    // segmento responsável por exibir o buffer;
    int j = 0;

    for (j; j < N; j++){
        printf("%d ", buffer[j]);
    }
    printf("\n\n");
    // terminando segmento de exibição de buffer;

    // entrando na região crítica;
    // adicionando item ao buffer;
    buffer[0] = item;
    
    if (*mutex == 0){
        signal(empty);
    }
}

void remover(void){
    // referenciando o espaço de memória compartilhada;
    mutex = shmat(shmid1, NULL, 0);
    empty = shmat(shmid2, NULL, 0);
    full = shmat(shmid3, NULL, 0);
    buffer = shmat(shmid4, NULL, 0);

    if (*mutex == 0){
        wait(empty);
    }

    int index = 1;

    buffer[0] = 0;

    while (index < N){
        buffer[index - 1] = buffer[index];
        index++;
    }

    buffer[index] = 0;

    // decrementando váriavel de controle de acesso à região crítica;
    (*mutex)++;

    // segmento responsável por exibir o buffer;
    int j = 0;

    for (j; j < N; j++){
        printf("%d ", buffer[j]);
    }
    printf("\n\n");
    // terminando segmento de exibição de buffer;

    if (*mutex == N){
        signal(full);
    }
}

void producer(void){
    int item, i = 0;
    
    srand(time(NULL));

    while (i < N){
        printf("Produtor: Estou produzindo um item...\n");
        item = (rand() % 10) + 1;
        insert(item);
        
        sleep(3);

        i++;
    }
}

void consumer(void){
    int i = 0;
    
    while(i < N){
        printf("Consumidor: Estou consumindo um item...\n");
        remover();

        sleep(5);

        i++;
    }
}

int main(void){
    // alocando o espaço de memória compartilhada;
    shmid1 = shmget(k1, sizeof(semaphore), IPC_CREAT | 0660); // mutex;
    shmid2 = shmget(k2, sizeof(semaphore), IPC_CREAT | 0660); // empty;
    shmid3 = shmget(k3, sizeof(semaphore), IPC_CREAT | 0660); // full;
    shmid4 = shmget(k4, sizeof(int) * N, IPC_CREAT | 0660); // buffer;

    // verficando se os endereços estão disponíveis para indexação;
    // caso não, ele entrega uma mensagem de error.
    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0){
        perror("main shmget error.");
        exit(1);
    }
    
    // inicializando mutex como zero, pois o buffer começa vazio;
    int mtx = 0, emp = N, fll = 0;

    mutex = (semaphore*)shmat(shmid1, NULL, 0);
    empty = (semaphore*)shmat(shmid2, NULL, 0);
    full = (semaphore*)shmat(shmid3, NULL, 0);

    mutex = &mtx;
    empty = &emp;
    full = &fll;

    // inicializando todos os espaços do buffer em zero;
    buffer = (int*)shmat(shmid3, NULL, 0);

    int i = 0;

    while (i < N){
        buffer[i] = 0;
        i++;
    }

    if (fork() == 0){
        consumer();
    } else {
        producer();
    }

    // desvinculando os espaços de memória compartilhada;
    shmdt(mutex);
    shmdt(empty);
    shmdt(full);
    shmdt(buffer);

    // desalocando memória compartilhada;
    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    shmctl(shmid3, IPC_RMID, NULL);
    shmctl(shmid4, IPC_RMID, NULL);

    return 0;
}