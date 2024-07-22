#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "osPRNG.h"
#include <stdbool.h>

#define MAXIMO_PRIORIDADE 7
#define MAXIMO_PROCESSOS 100
#define PROBABILIDADE_IO_REQ 10
#define PROBABILIDADE_TERMINAR_IO_REQ 4
#define NUM_FILAS 3

float AGGING_FACTOR;

typedef struct {
    int pid;
    int tempoChegada;
    int clocksNecessarios;
    int prioridade;
    int tempoRestante;
    int tempoIO;
    int emIO;
    int tempoReady;
    int tempoWait;
    int tempoSistema;
    float estimativaTempoRestante;
} Processo;

int IO_Req() {
    if( osPRNG() % PROBABILIDADE_IO_REQ == 0 )
        return 1;
    else
        return 0;
}

int IO_Term(){
    if( osPRNG() % PROBABILIDADE_TERMINAR_IO_REQ == 0 )
        return 1;
    else
        return 0;
}

int lerProcessosDoArquivo(const char *nomeArquivo, Processo processos[], int maxProcessos) {
    FILE *arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        perror("Não foi possível abrir o arquivo");
        return -1;
    }

    int i = 0;
    while (i < maxProcessos && fscanf(arquivo, "%d:%d:%d:%d\n", 
                                      &processos[i].pid,
                                      &processos[i].tempoChegada,
                                      &processos[i].clocksNecessarios,
                                      &processos[i].prioridade) == 4) {
        processos[i].tempoRestante = processos[i].clocksNecessarios;
        processos[i].tempoIO = 0;
        processos[i].emIO = 0;
        processos[i].tempoReady = 0;
        processos[i].tempoWait = 0;
        processos[i].tempoSistema = 0;

        i++;
    }

    fclose(arquivo);
    return i;
}

void ordenarPorTempoRestante(Processo processos[], int numProcessos) {
    int i, j;
    Processo temp;
    
    for (i = 0; i < numProcessos - 1; i++) {
        for (j = 0; j < numProcessos - i - 1; j++) {
            if (processos[j].estimativaTempoRestante > processos[j + 1].estimativaTempoRestante) {
                temp = processos[j];
                processos[j] = processos[j + 1];
                processos[j + 1] = temp;
            }
        }
    }
}

void imprimirStatus(int tempoAtual, int pid, int tempoRestante, const char *status, int flagV) {
    if (flagV) {
        if(strcmp(status, "wait") == 0) {
            fprintf(stderr, "%d:%d:%d:true:null:%s\n", tempoAtual, pid, tempoRestante, status);
        } else {
            fprintf(stderr, "%d:%d:%d:false:null:%s\n", tempoAtual, pid, tempoRestante, status);
        }
        // fprintf(stderr, "%d:%d:%d:false:null:%s\n", tempoAtual, pid, tempoRestante, status);
    }
}

void escalonadorSJFPreemptivo(Processo processos[], int numProcessos, int flagV) {
    int tempoAtual = 0;
    int processosPendentes = numProcessos;
    int processoExecutado = 1;
    int chegouProcesso = 0;

    Processo* processoChegada;
    Processo* ultimoProcesso = &processos[0];

    for (int i = 0; i < numProcessos; i++) {
        processos[i].estimativaTempoRestante = processos[i].tempoRestante;
    }

    while (processosPendentes != 0) {
        ordenarPorTempoRestante(processos, numProcessos);

        if (processoExecutado == 0) {
            if (flagV) {
                fprintf(stderr, "%d:null:null:false:idle\n", tempoAtual);
            }
            if (processosPendentes == -1) {
                processosPendentes = 0;
                break;
            }
        }

        chegouProcesso = 0;

        for (int i = 0; i < numProcessos; i++) {
            if (processos[i].tempoChegada == tempoAtual) {
                chegouProcesso = 1;
                processoChegada = &processos[i];
                break;
            }
        }

        processoExecutado = 0;
        for (int i = 0; i < numProcessos; i++) {

            if (!chegouProcesso) {
                ultimoProcesso = &processos[i];
            }

            if (chegouProcesso) {
                if (processos[i].tempoChegada == tempoAtual) {
                    Processo* processoTestado = &processos[i];
                    if (ultimoProcesso != processoChegada) {
                        imprimirStatus(tempoAtual, ultimoProcesso->pid, ultimoProcesso->tempoRestante, "preempted", flagV);
                        imprimirStatus(tempoAtual, processoChegada->pid, processoChegada->tempoRestante, "new", flagV);
                        ultimoProcesso->tempoSistema++;
                        processoChegada->tempoSistema++;
                    } else {
                        imprimirStatus(tempoAtual, processos[i].pid, processos[i].tempoRestante, "new", flagV);
                        processos[i].tempoSistema++;
                    }
                    processoExecutado = 1;
                    break;
                }
            } else if (processos[i].tempoChegada <= tempoAtual && processos[i].tempoRestante > 0) {
                if (processos[i].emIO) {
                    processos[i].tempoIO++;
                    if (IO_Term()) {
                        processos[i].emIO = 0;
                        processos[i].tempoReady++;
                        processos[i].tempoSistema++;
                        imprimirStatus(tempoAtual, processos[i].pid, processos[i].tempoRestante, "ready", flagV);
                    } else {
                        processos[i].tempoWait++;
                        processos[i].tempoSistema++;
                        imprimirStatus(tempoAtual, processos[i].pid, processos[i].tempoRestante, "wait", flagV);
                    }
                    processoExecutado = 1;
                    break;
                } else if (IO_Req() && processos[i].emIO == 0) {
                    processos[i].emIO = 1;
                    processos[i].tempoIO++;
                    processos[i].tempoSistema++;
                    imprimirStatus(tempoAtual, processos[i].pid, processos[i].tempoRestante, "wait", flagV);
                    processos[i].tempoWait++;
                    processoExecutado = 1;
                    break;
                } else {
                    processos[i].estimativaTempoRestante = (AGGING_FACTOR * (processos[i].tempoRestante - 1)) + ((1 - AGGING_FACTOR) * processos[i].estimativaTempoRestante);
                    processos[i].tempoRestante -= 1;
                    processos[i].tempoSistema++;

                    if (processos[i].tempoRestante == 0) {
                        imprimirStatus(tempoAtual, processos[i].pid, processos[i].tempoRestante, "end", flagV);
                        processosPendentes--;
                    } else {
                        imprimirStatus(tempoAtual, processos[i].pid, processos[i].tempoRestante, "running", flagV);
                    }

                    processoExecutado = 1;
                    break;
                }
            }
        }

        if (processosPendentes == 0) {
            processosPendentes = -1;
            processoExecutado = 0;
        }

        tempoAtual++;
    }

    printf("=========+=================+=================+============\n");
    printf("Processo | Tempo total     | Tempo total     | Tempo total\n");
    printf("         | em estado Ready | em estado Wait  | no sistema\n");
    printf("=========+=================+=================+============\n");
    for (int i = 0; i < numProcessos; i++) {
        printf(" %2d     | %2d              | %2d              | %2d\n",
               processos[i].pid,
               processos[i].tempoReady,
               processos[i].tempoWait,
               processos[i].tempoSistema);
    }
    printf("=========+=================+=================+============\n");

    int tempoSimulacao = tempoAtual;
    int menorTempo = INT_MAX;
    int maiorTempo = 0;
    int somaTempo = 0;

    int totalReadyWait = 0;

    for (int i = 0; i < numProcessos; i++) {
        int tempoExecucao = processos[i].tempoSistema;
        somaTempo += tempoExecucao;
        if (tempoExecucao < menorTempo) menorTempo = tempoExecucao;
        if (tempoExecucao > maiorTempo) maiorTempo = tempoExecucao;
        totalReadyWait += (processos[i].tempoReady + processos[i].tempoWait);
    }
    float tempoMedioExecucao = (float) somaTempo / numProcessos;
    float tempoMedioReadyWait = (float) (totalReadyWait) / numProcessos;

    printf("Tempo total de simulação.: %d (somado +1 pelo inicio em 0 )\n", tempoSimulacao += 1);
    printf("Número de processos......: %d\n", numProcessos);
    printf("Menor tempo de execução..: %d\n", menorTempo);
    printf("Maior tempo de execução..: %d\n", maiorTempo);
    printf("Tempo médio de execução..: %.2f\n", tempoMedioExecucao);
    printf("Tempo médio em Ready/Wait: %.2f\n", tempoMedioReadyWait);
}

void ordenarPorPrioridade(Processo processos[], int numProcessos) {
    for (int i = 0; i < numProcessos - 1; i++) {
        for (int j = i + 1; j < numProcessos; j++) {
            if (processos[i].prioridade < processos[j].prioridade) {
                Processo temp = processos[i];
                processos[i] = processos[j];
                processos[j] = temp;
            }
        }
    }
}

int quantumTime(int prioridade) {
    return (1 << prioridade);
}

void escalonadorMultiplasFilas(Processo processos[], int numProcessos, int flagV) {
    Processo filas[NUM_FILAS][MAXIMO_PROCESSOS];
    int numFilas[NUM_FILAS] = {0};
    int tempoAtual = 0;
    int processosPendentes = numProcessos;
    int processoExecutado = 1;

    for (int i = 0; i < numProcessos; i++) {
        int prioridade = processos[i].prioridade;
        if (prioridade < NUM_FILAS) {
            filas[prioridade][numFilas[prioridade]++] = processos[i];
        }
    }

    while (processosPendentes > 0) {
        if (processoExecutado == 0) {
            if (flagV) {
                fprintf(stderr, "%d:null:null:false:idle\n", tempoAtual);
            }
        }

        processoExecutado = 0;

        for (int fila = NUM_FILAS - 1; fila >= 0; fila--) {
            int quantum = quantumTime(fila);
            for (int i = 0; i < numFilas[fila]; i++) {
                Processo p = filas[fila][i];
                int tempoExecucao = 0;
                while (p.tempoRestante > 0 && tempoExecucao < quantum) {
                    if (p.emIO) {
                        p.tempoIO++;
                        if (IO_Term()) {
                            p.emIO = 0;
                            p.tempoReady++;
                            p.tempoSistema++;
                            imprimirStatus(tempoAtual, p.pid, p.tempoRestante, "ready", flagV);
                        } else {
                            p.tempoWait++;
                            p.tempoSistema++;
                            imprimirStatus(tempoAtual, p.pid, p.tempoRestante, "wait", flagV);
                        }
                        processoExecutado = 1;
                        break;
                    } else if (IO_Req() && p.emIO == 0) {
                        p.emIO = 1;
                        p.tempoIO++;
                        p.tempoSistema++;
                        imprimirStatus(tempoAtual, p.pid, p.tempoRestante, "wait", flagV);
                        p.tempoWait++;
                        processoExecutado = 1;
                        break;
                    } else {
                        p.tempoRestante -= 1;
                        p.tempoSistema++;

                        if (p.tempoRestante == 0) {
                            imprimirStatus(tempoAtual, p.pid, p.tempoRestante, "end", flagV);
                            processosPendentes--;
                        } else {
                            imprimirStatus(tempoAtual, p.pid, p.tempoRestante, "running", flagV);
                        }

                        processoExecutado = 1;
                    }

                    tempoExecucao++;
                }

                if (p.tempoRestante > 0) {
                    Processo temp = p;
                    for (int j = i; j < numFilas[fila] - 1; j++) {
                        filas[fila][j] = filas[fila][j + 1];
                    }
                    filas[fila][numFilas[fila] - 1] = temp;
                } else {
                    for (int j = i; j < numFilas[fila] - 1; j++) {
                        filas[fila][j] = filas[fila][j + 1];
                    }
                    numFilas[fila]--;
                    i--;
                }

                if (processoExecutado) break;
            }
            if (processoExecutado) break;
        }

        tempoAtual++;
    }

    printf("=========+=================+=================+============\n");
    printf("Processo | Tempo total     | Tempo total     | Tempo total\n");
    printf("         | em estado Ready | em estado Wait  | no sistema\n");
    printf("=========+=================+=================+============\n");
    for (int i = 0; i < numProcessos; i++) {
        printf(" %2d     | %2d              | %2d              | %2d\n",
               processos[i].pid,
               processos[i].tempoReady,
               processos[i].tempoWait,
               processos[i].tempoSistema);
    }
    printf("=========+=================+=================+============\n");

    int tempoSimulacao = tempoAtual;
    int menorTempo = INT_MAX;
    int maiorTempo = 0;
    int somaTempo = 0;
    int totalReadyWait = 0;

    for (int i = 0; i < numProcessos; i++) {
        Processo* processo = &processos[i];
        printf("Processo %d: tempoRestante=%d, tempoReady=%d, tempoWait=%d, tempoSistema=%d\n",
               processo->pid, processo->tempoRestante, processo->tempoReady, processo->tempoWait, processo->tempoSistema);
        int tempoExecucao = processo->tempoSistema;
        somaTempo += tempoExecucao;
        if (tempoExecucao < menorTempo) menorTempo = tempoExecucao;
        if (tempoExecucao > maiorTempo) maiorTempo = tempoExecucao;
        totalReadyWait += (processo->tempoReady + processo->tempoWait);
    }

    float tempoMedioExecucao = (float) somaTempo / numProcessos;
    float tempoMedioReadyWait = (float) (totalReadyWait) / numProcessos;

    printf("Tempo total de simulação.: %d (somado +1 pelo inicio em 0 )\n", tempoSimulacao + 1);
    printf("Número de processos......: %d\n", numProcessos);
    printf("Menor tempo de execução..: %d\n", menorTempo);
    printf("Maior tempo de execução..: %d\n", maiorTempo);
    printf("Tempo médio de execução..: %.2f\n", tempoMedioExecucao);
    printf("Tempo médio em Ready/Wait: %.2f\n", tempoMedioReadyWait);
}

bool MultipleQueues;
bool ShortestJobFirst;

void processarArgumentos(int argc, char *argv[], char **nomeArquivo, int *flagV, int *flagF, float *aggingFactor, int *escalonadorTipo) {
    *flagV = 0;
    *flagF = 0;
    *aggingFactor = 0.0f;
    *escalonadorTipo = 0;

    for (int i = 1; i < argc; i++) {
        printf("Argumento: %s\n", argv[i]);
        if (strcmp(argv[i], "-v") == 0) {
            *flagV = 1;
        } else if (strcmp(argv[i], "-F") == 0) {
            MultipleQueues = true;
            *flagF = 1;
        } else if (strcmp(argv[i], "-S") == 0) {
            if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                *aggingFactor = atof(argv[++i]);
                AGGING_FACTOR = *aggingFactor;
            }
            ShortestJobFirst = true;
        } else {
            *nomeArquivo = argv[i];
        }
    }
}

int main(int argc, char *argv[]) {
    char *nomeArquivo = NULL;
    int flagV, flagF;
    float aggingFactor;
    int escalonadorTipo;

    processarArgumentos(argc, argv, &nomeArquivo, &flagV, &flagF, &aggingFactor, &escalonadorTipo);

    if (nomeArquivo == NULL) {
        fprintf(stderr, "Uso: %s <arquivo> [-v] [-F] [-S [<valor float>]]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Processo processos[MAXIMO_PROCESSOS];
    int numProcessos = lerProcessosDoArquivo(nomeArquivo, processos, MAXIMO_PROCESSOS);
    if (numProcessos < 0) {
        fprintf(stderr, "Erro ao ler processos do arquivo.\n");
        return EXIT_FAILURE;
    }

    printf("Lidos %d processos do arquivo %s\n", numProcessos, nomeArquivo);

    if (flagV) {
        printf("Flag -v ativada.\n");
    }
    if (MultipleQueues) {
        escalonadorTipo = 2;
        printf("Flag -F ativada.\n");
    }
    if (ShortestJobFirst) {
        escalonadorTipo = 1;
        printf("Flag -S ativada com valor: %.2f\n", aggingFactor);
    }

    if (ShortestJobFirst) {
        escalonadorSJFPreemptivo(processos, numProcessos, flagV);
    } else if (MultipleQueues) {
        escalonadorMultiplasFilas(processos, numProcessos, flagV);
    } else {
        for (int i = 0; i < numProcessos; i++) {
            printf("PID: %d, Tempo Chegada: %d, Clocks Necessários: %d, Prioridade: %d\n",
                   processos[i].pid,
                   processos[i].tempoChegada,
                   processos[i].clocksNecessarios,
                   processos[i].prioridade);
        }
    }

    return EXIT_SUCCESS;
}
