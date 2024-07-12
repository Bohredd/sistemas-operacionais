#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES 100
#define MAX_PRIORITY 8

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_burst;
    int state;  // 0: Ready, 1: Running, 2: Wait
    int total_ready_time; // Total de tempo em estado Ready
    int total_wait_time;  // Total de tempo em estado Wait
    int total_running_time; // Total de tempo em estado Running
} Process;

typedef struct {
    Process *queue[MAX_PRIORITY][MAX_PROCESSES]; // Fila de prioridade
    int size[MAX_PRIORITY];
} PriorityQueues;

typedef struct {
    int total_processes;
    int total_elapsed_time;
    int total_throughput;
    int total_ready_time;
    int total_wait_time;
    int total_running_time;
    int min_execution_time;
    int max_execution_time;
} Statistics;

typedef enum {
    FMP,  // Filas Múltiplas com Prioridade
    SJF   // Shortest Job First preemptivo
} SchedulerPolicy;

// Funções para o escalonador
void schedule_processes(Process processes[], int num_processes, SchedulerPolicy policy, FILE *verbose_output, int clock, PriorityQueues queues);

// Funções de I/O simuladas
#define PROB_OF_IO_REQ 10
#define PROB_OF_IO_TERM 4

int osPRNG() {
    return rand();  // Função de gerador de números aleatórios simulada
}

int IOReq() {
    if (osPRNG() % PROB_OF_IO_REQ == 0)
        return 1;
    else
        return 0;
}

int IOTerm() {
    if (osPRNG() % PROB_OF_IO_TERM == 0)
        return 1;
    else
        return 0;
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <inputfile.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    Process processes[MAX_PROCESSES];
    int num_processes = 0;

    // Leitura dos processos
    while (fscanf(file, "%d:%d:%d:%d\n", &processes[num_processes].pid,
                  &processes[num_processes].arrival_time,
                  &processes[num_processes].burst_time,
                  &processes[num_processes].priority) != EOF) {
        processes[num_processes].remaining_burst = processes[num_processes].burst_time;
        processes[num_processes].state = 0; // Inicia todos os processos como Ready
        processes[num_processes].total_ready_time = 0;
        processes[num_processes].total_wait_time = 0;
        processes[num_processes].total_running_time = 0;
        num_processes++;
    }

    fclose(file);

    // Inicialização das estruturas de dados e estatísticas
    PriorityQueues queues = {0};
    Statistics stats = {0};
    FILE *verbose_output = NULL;

    // Verificação do modo verbose
    if (argc > 2 && strcmp(argv[2], "-v") == 0) {
        verbose_output = stderr;
    }

    // Loop principal do Supervisor
    int clock = 0;
    bool processes_running = true;
    while (processes_running) {
        // Adicionar novos processos ao estado Ready
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].arrival_time == clock) {
                queues.queue[processes[i].priority][queues.size[processes[i].priority]] = &processes[i];
                queues.size[processes[i].priority]++;
                processes[i].state = 0; // Ready
                stats.total_processes++;
            }
        }

        // Chamar o escalonador para determinar o próximo processo a executar
        schedule_processes(processes, num_processes, FMP, verbose_output, clock, queues);

        // Verificar se algum processo está em execução
        processes_running = false;
        for (int p = 0; p < MAX_PRIORITY; p++) {
            if (queues.size[p] > 0) {
                processes_running = true;
                break;
            }
        }

        // Simular o clock tick
        clock++;
    }

    // Calcular estatísticas finais
    stats.total_elapsed_time = clock - 1; // Último clock incrementado antes de sair do loop

    int total_turnaround_time = 0;
    int total_ready_time = 0;
    int total_wait_time = 0;
    int total_running_time = 0;
    stats.min_execution_time = processes[0].burst_time;
    stats.max_execution_time = processes[0].burst_time;

    for (int i = 0; i < num_processes; i++) {
        total_turnaround_time += (clock - processes[i].arrival_time);
        total_ready_time += processes[i].total_ready_time;
        total_wait_time += processes[i].total_wait_time;
        total_running_time += processes[i].total_running_time;

        if (processes[i].burst_time < stats.min_execution_time) {
            stats.min_execution_time = processes[i].burst_time;
        }
        if (processes[i].burst_time > stats.max_execution_time) {
            stats.max_execution_time = processes[i].burst_time;
        }
    }

    stats.total_throughput = num_processes;
    stats.total_ready_time = total_ready_time;
    stats.total_wait_time = total_wait_time;
    stats.total_running_time = total_running_time;

    // Imprimir estatísticas gerais
    printf("=========+=================+=================+============\n");
    printf("Processo | Tempo total     | Tempo total     | Tempo total\n");
    printf("         | em estado Ready | em estado Wait  | no sistema\n");
    printf("=========+=================+=================+============\n");
    for (int i = 0; i < num_processes; i++) {
        printf(" %-8d | %-15d | %-15d | %-10d\n", processes[i].pid, processes[i].total_ready_time,
               processes[i].total_wait_time, clock - processes[i].arrival_time);
    }
    printf("=========+=================+=================+============\n");
    printf("Tempo total de simulação.: %d\n", stats.total_elapsed_time);
    printf("Número de processos......: %d\n", stats.total_processes);
    printf("Menor tempo de execução..: %d\n", stats.min_execution_time);
    printf("Maior tempo de execução..: %d\n", stats.max_execution_time);
    printf("Tempo médio de execução..: %d\n", total_turnaround_time / num_processes);
    printf("Tempo médio em Ready/Wait: %d\n", (total_ready_time + total_wait_time) / num_processes);
    printf("=========+=================+=================+============\n");

    return EXIT_SUCCESS;
}

// Implementação do escalonador
void schedule_processes(Process processes[], int num_processes, SchedulerPolicy policy, FILE *verbose_output, int clock, PriorityQueues queues) {
    switch (policy) {
        case FMP:
            // Implementação do escalonamento por Filas Múltiplas com Prioridade
            for (int p = 0; p < MAX_PRIORITY; p++) {
                if (queues.size[p] > 0) {
                    Process *next_process = queues.queue[p][0];
                    next_process->state = 1; // Running

                    // Simulação da execução do processo
                    int quantum = 1 << p; // Quantum baseado na prioridade
                    for (int t = 0; t < quantum && next_process->remaining_burst > 0; t++) {
                        // Verbose output
                        if (verbose_output != NULL) {
                            fprintf(verbose_output, "%d:%d:%d:%d:%d:%s\n", clock, next_process->pid,
                                    next_process->remaining_burst - 1, IOReq(), IOTerm(),
                                    "running");
                        }

                        next_process->remaining_burst--;
                        next_process->total_running_time++;
                    }

                    if (next_process->remaining_burst <= 0) {
                        next_process->state = 2; // Wait
                        next_process->total_wait_time++;
                    } else {
                        next_process->state = 0; // Ready
                        next_process->total_ready_time++; // Incrementa o tempo total em Ready
                        queues.size[p]--;
                    }

                    // Remover o processo da fila de prioridade
                    for (int i = 0; i < queues.size[p]; i++) {
                        queues.queue[p][i] = queues.queue[p][i + 1];
                    }
                    break; // Processo executado, sair do loop
                }
            }
            break;

        case SJF:
            // Implementação do escalonamento por Shortest Job First preemptivo
            // Ordenar processos por ordem crescente de burst_time
            for (int i = 0; i < num_processes - 1; i++) {
                for (int j = i + 1; j < num_processes; j++) {
                    if (processes[i].burst_time > processes[j].burst_time) {
                        // Trocar os processos de posição
                        Process temp = processes[i];
                        processes[i] = processes[j];
                        processes[j] = temp;
                    }
                }
            }

            // Executar o primeiro processo na fila, o mais curto
            Process *shortest_job = &processes[0];
            shortest_job->state = 1; // Running

            // Simulação da execução do processo
            while (shortest_job->remaining_burst > 0) {
                // Verbose output
                if (verbose_output != NULL) {
                    fprintf(verbose_output, "%d:%d:%d:%d:%d:%s\n", clock, shortest_job->pid,
                            shortest_job->remaining_burst - 1, IOReq(), IOTerm(),
                            "running");
                }

                shortest_job->remaining_burst--;
                shortest_job->total_running_time++;
                clock++; // Incrementar o clock para cada unidade de burst_time
            }

            if (shortest_job->remaining_burst <= 0) {
                shortest_job->state = 2; // Wait
                shortest_job->total_wait_time++;
            } else {
                shortest_job->state = 0; // Ready
                shortest_job->total_ready_time++; // Incrementa o tempo total em Ready
            }

            break;

        default:
            fprintf(stderr, "Invalid scheduler policy\n");
            break;
    }
}
