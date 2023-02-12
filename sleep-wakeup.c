#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <stdbool.h>

// Tamanho do BUFFER
#define BUFFER_SIZE 10
// Essas definições indicam os IDs do produtor e do consumidor;
#define produce 0
#define consume 1

pid_t consumer_id, producer_id;

// definindo as flags e o buffer;
int *buffer;
bool *flag;
int *turn;
key_t k1 = 0003, k2 = 4000, k3 = 5000, k4 = 4121, k5 = 0127;

pid_t *pid_consumidor;
pid_t *pid_produtor;
pid_t process_id;

int shmid1, shmid2, shmid3, shmid4, shmid5;

void handle_signal(int signal);
void enter_region(int process);
void leave_region(int process);
void producer();
void consumer();

void handle_signal(int signal)
{
    printf("Signal recebido: %d\n", signal);
}

void wake_up(pid_t pid)
{
    kill(pid, SIGCONT);
}

void SLEEP()
{
    process_id = getpid();
    printf("a mimir\n");
    kill(process_id, SIGSTOP);
}

void enter_region(int process)
{
    // referenciando espaço de memória compartilhada;
    flag = (bool *)shmat(shmid1, NULL, 0);
    turn = (int *)shmat(shmid2, NULL, 0);

    int other;

    other = 1 - process;
    flag[process] = true;
    *turn = process;

    // coloca o processo para dormir caso a regiao crítica esteja ocupada;
    while (*turn == process && flag[other])
    {
        sleep(1);
    }
}

// função que cuida da saída do processo da região crítica;
void leave_region(int process)
{
    // referenciando espaço de memória compartilhada;
    flag = (bool *)shmat(shmid1, NULL, 0);

    // segmento que controla a saída da região crítica;
    flag[process] = false;
}

void producer()
{
    buffer = (int *)shmat(shmid3, NULL, 0);
    producer_id = getpid();
    if (buffer == (void *)-1)
    {
        perror("Shared memory attach");
    }

    while (1)
    {
        if (buffer[BUFFER_SIZE - 1] != 0)
        {
            SLEEP();
        }
        else
        {
            enter_region(produce);
            printf("PRODUTOR: ");
            int item, index = 0;
            item = (rand() % 10) + 1;

            // ADICIONANDO ITEM NO BUFFER

            while (index < BUFFER_SIZE)
            {
                if (buffer[index] == 0)
                {
                    buffer[index] = item;
                    break;
                }
                index++;
            }

            int j = 0;

            for (j; j < BUFFER_SIZE; j++)
            {
                printf("%d ", buffer[j]);
            }
            printf("\n");
            leave_region(produce);
        }

        if (buffer[0] != 0)
        {
            wake_up(*pid_produtor);
        }
        sleep(2);
    }
}

void consumer()
{

    // referenciando o espaço de memória compartilhada;
    buffer = (int *)shmat(shmid3, NULL, 0);
    consumer_id = getpid();
    while (1)
    {
        if (buffer == (void *)-1)
        {
            perror("Shared memory attach");
        }

        if (buffer[0] == 0)
        {
            SLEEP();
        }
        else
        {
            enter_region(consume);
            printf("CONSUMIDOR: ");

            buffer[0] = 0;
            int index = 1;

            while (index < BUFFER_SIZE)
            {
                buffer[index - 1] = buffer[index];
                index++;
            }

            buffer[index] = 0;
            int j = 0;

            for (j; j < BUFFER_SIZE; j++)
            {
                printf("%d ", buffer[j]);
            }
            printf("\n");
            leave_region(consume);
        }

        if (buffer[BUFFER_SIZE - 1] == 0)
        {
            wake_up(*pid_produtor);
        }
    }
}

int main()
{
    shmid1 = shmget(k1, sizeof(int) * BUFFER_SIZE, IPC_CREAT | 0600);
    shmid2 = shmget(k2, sizeof(int) * 2, IPC_CREAT | 0600);
    shmid3 = shmget(k3, sizeof(bool) * 2, IPC_CREAT | 0600);
    shmid4 = shmget(k4, sizeof(int), 0600 | IPC_CREAT);
    shmid5 = shmget(k5, sizeof(int), 0600 | IPC_CREAT);

    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0)
    {
        perror("Erro em shmget!");
        exit(1);
    }

    buffer = (int *)shmat(shmid1, NULL, 0);
    pid_consumidor = (int *)shmat(shmid2, NULL, 0);
    pid_produtor = (int *)shmat(shmid3, NULL, 0);
    // inicializando todas as posições do buffer em 0;
    int i = 0;

    while (i < BUFFER_SIZE)
    {
        buffer[i] = 0;
        i++;
    }

    pid_t pid = fork();
    // usamos a chamada fork para criar dois processos executanto paralelamente;
    if (pid < 0)
    {
        perror("fork fail");
        exit(1);
    }
    else if (pid == 0) // Child Process - Producer
    {
        *pid_consumidor = getppid();
        producer();
    }
    else // Parent Process
    {
        *pid_produtor = pid;
        consumer();
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
