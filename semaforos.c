#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <time.h>

#define N 10

// definindo as chaves e as variáveis de ID para a memória compartilhada;
key_t k1 = 5491, k2 = 5812, k3 = 4327, k4 = 3213;
int shmid1, shmid2, shmid3, shmid4;

// definindo os semáforos que serão utilizados bem como o buffer;
typedef int semaphore;
semaphore *mutex;
semaphore *empty;
semaphore *full;
int *buffer;

// função wait() decrementa o semáforo caso possível;
//senão ele dorme até que seja possível;
void wait(semaphore *s){
    while (s <= 0){
        sleep(5);
    }
    (*s)--;
}

// função signal() incrementa o semáforo;
void signal(semaphore *s){
    (*s)++;
}

// Função que abstrai o papel do consumidor;
// A função produz um número aleatório entre 1 e 10;
void producer(){
    // referenciando o espaço de memória compartilhada;
    mutex = (semaphore*)shmat(shmid1, NULL, 0);
    empty = (semaphore*)shmat(shmid2, NULL, 0);
    full = (semaphore*)shmat(shmid3, NULL, 0);
    buffer = (int*)shmat(shmid4, NULL, 0);

    // variável na qual será armezanado o item produzido;
    // a variável i serve de controle para determinar quantas vezes o produtor irá agir;
    int item, index = 0, i = 0;

    // gerando uma seed para rand baseada no tempo atual;
    srand(time(NULL));

    while (i < N){
        // produzindo um número aleatório para armazenar no buffer.
        item  = (rand() % 10) + 1;

        wait(empty); // verificando se há espaço no buffer;
        wait(mutex); // entrando na região crítica;

        printf("Estou produzindo um item...\n");

        // o segmento abaixo adiciona o item a última primeira posição livre dentro do buffer;
        while (index < N){
            
            if (buffer[index] == 0){
                buffer[index] = item; // item armazenado no buffer;
                break;
            }

            index++;
        }

        // este segmento é apenas para exibir os valores do buffer;
        int j = 0;

        for (j; j < N; j++){
            printf("%d ", buffer[j]);
        }
        printf("\n\n");
        // terminando segmento de exibição de valores

        signal(mutex); // saindo da região crítica;
        signal(full); // acrescentando um espaço preenchido no buffer;

        i++;

        sleep(3);
    }
}

void consumer(){
    // referenciando o espaço de memória compartilhada;
    mutex = (semaphore*)shmat(shmid1, NULL, 0);
    empty = (semaphore*)shmat(shmid2, NULL, 0);
    full = (semaphore*)shmat(shmid3, NULL, 0);
    buffer = (int*)shmat(shmid4, NULL, 0);
    
    // a variável i controla a quantidade de vezes que o consumidor agirá;
    int i = 0;
    
    while (i < N){
        wait(full); // indica que a itens no buffer para serem consumidos; 
        wait(mutex); // entrando na região crítica;

        printf("Estou consumindo um item...\n");

        // o segmento abaixo é responsável pelo consumo do primeiro produzido;
        buffer[0] = 0;

        int index = 1;
        
        while (index < N){
            buffer[index - 1] = buffer[index];
            index++;
        }

        buffer[index] = 0;
        // segmento responsável por exibir o buffer;
        int j = 0;

        for (j; j < N; j++){
            printf("%d ", buffer[j]);
        }
        printf("\n\n");
        // terminando segmento de exibição de buffer;

        signal(mutex); // saindo da região crítica;
        signal(empty); // liberando um espaço no buffer;

        i++;

        sleep(5);
    }
}

int main(void){
    // pegando referências para os espaços de memória compartilhados;
    shmid1 = shmget(k1, sizeof(semaphore), IPC_CREAT | 0660); // mutex
    shmid2 = shmget(k2, sizeof(semaphore), IPC_CREAT | 0660); // empty
    shmid3 = shmget(k3, sizeof(semaphore), IPC_CREAT | 0660); // full
    shmid4 = shmget(k4, sizeof(int) * N, IPC_CREAT | 0660); // buffer
    
    // verificando se esses endereços estão disponíveis para alocação;
    // caso não, ele devolve uma mensagem de erro;
    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0 || shmid4 < 0){
        perror("main shmget error.\n\n");
        exit(1);
    }

    // iniciando os valores dos semáforos;
    int mut = 1, emp = N, fll = 0;

    // referenciando o espaço de memória compartilhada;
    mutex = (semaphore*)shmat(shmid1, NULL, 0);
    empty = (semaphore*)shmat(shmid2, NULL, 0);
    full = (semaphore*)shmat(shmid3, NULL, 0);
    buffer = (int*)shmat(shmid4, NULL, 0);

    // armazenando os valores dos semáforos;
    mutex = &mut;
    empty = &emp;
    full = &fll;

    // inicializando o buffer com zeros;
    int i = 0;

    while (i < N){
        buffer[i] = 0;
        i++;
    }

    // usando a chamada fork para gerar dois processos que rodam em paralelo;
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