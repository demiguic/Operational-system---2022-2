#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <time.h>

// Essas definições indicam os IDs do produtor e do consumidor;
#define produce 0
#define consume 1

// Definindo o tamanho do buffer;
#define N 10

// constante que vai definir o tempo de exercução do programa;
#define runtime 10

// definindo as chaves e as variáveis de ID para a memória compartilhada;
key_t k1 = 1234, k2 = 2345, k3 = 3456, k4 = 4567;
int shmid1, shmid2, shmid3, shmid4;

// definindo as flags e o buffer;
bool *flag;
int *turn;
int *buffer;

// função que cuida da entrada do processo na região crítica;
void enter_region(int process){
    // referenciando espaço de memória compartilhada;
    flag = (bool *)shmat(shmid1, NULL, 0);
    turn = (int *)shmat(shmid2, NULL, 0);

    int other;

    other = 1 - process;
    flag[process] = true;
    *turn = process;

    // coloca o processo para dormir caso a regiao crítica esteja ocupada;
    while (*turn == process && flag[other]){
        sleep(1);
    }
}

// função que cuida da saída do processo da região crítica;
void leave_region(int process){
    // referenciando espaço de memória compartilhada;
    flag = (bool *)shmat(shmid1, NULL, 0);

    // segmento que controla a saída da região crítica;
    flag[process] = false;
}

// função que abstrai o papel do produtor;
void producer(){
    // referencindo o espaço de memória compartilhada;
    buffer = (int *)shmat(shmid3, NULL, 0);
    
    // condição verifica se o buffer está cheio;
    // caso esteja produtor não trabalha;
    if (buffer[N - 1] == 0){
        int item, index = 0;  

        // função gera uma seed para rand baseada em tempo;
        srand(time(NULL));

        printf("Estou Produzindo...\n");
        item = (rand() % 10) + 1;

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
    } else {
        printf("incapaz de produzir, buffer cheio!!!\n\n");
    }

    sleep(3);
}

// função que abstrai o papel do consumidor;
void consumer(){
    // referencindo o espaço de memória compartilhada;
    buffer = (int *)shmat(shmid3, NULL, 0);

    // condição verifica se buffer está vazio;
    // caso esteja, consumidor não consome;
    if (buffer[0] != 0){
        printf("Estou consumindo...\n");

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

    } else {
        printf("Incapaz de consumir algo, buffer vazio!!!\n\n");
    }

    sleep(5);
}

int main(void){
    // pegando referências para os espaços de memória compartilhados;
    shmid1 = shmget(k1, sizeof(bool) * 2, IPC_CREAT | 0666); // flag
    shmid2 = shmget(k2, sizeof(int) * 2, IPC_CREAT | 0666);  // turn
    shmid3 = shmget(k3, sizeof(int) * N, IPC_CREAT | 0666); // buffer

    // verificando se esses endereços estão disponíveis para alocação;
    // caso não, ele devolve uma mensagem de erro;
    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0){
        perror("Erro em shmget!");
        exit(1);
    }

    // referenciando o espaço de memória compartilhada do buffer;
    buffer = (int *)shmat(shmid3, NULL, 0);
    
    // inicializando todas as posições do buffer em 0;
    int i = 0;

    while (i < N){
        buffer[i] = 0;
        i++;
    }

    // usamos a chamada fork para criar dois processos executanto paralelamente;
    if (fork() == 0){
        int i = 0;

        while (i < runtime){
            enter_region(produce);
            consumer();
            leave_region(produce);
            i++;
        }

    } else {
        int j = 0;
        while (j < runtime){
            enter_region(consume);
            producer();
            leave_region(consume);
            j++;
        }

    }

    // desvinculando os espaços de memória compartilhada;
    shmdt(flag);
    shmdt(turn);
    shmdt(buffer);

    // desalocando memória compartilhada;
    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    shmctl(shmid3, IPC_RMID, NULL);

    return 0;
}