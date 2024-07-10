#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PROB_OF_IO_REQ 10
#define PROB_OF_IO_TERM 4

bool VERBOSITY = false;
bool MULTIPLEQUEUES = false;
bool FIRSTCOMEFIRSTSERVED = false;

float AGING_INTERVAL = 0.5;

int CLOCK = 0;

typedef struct Processo {
    int pid;
    int tempoChegada;
    int burst;
    int prioridadeProcesso;
    int tempoRestante;
    int tempoEspera;
    int tempoIO;
    int tempoTotal;
    char estadoAtual[20];
} Processo;

typedef struct Escalonador {
    int numProcessos;
    Processo *processos;
    int processosExecutados;
    int tempoSimulacao;
} Escalonador;

void liberarMemoria(Escalonador *escalonador){
    free(escalonador->processos);
}

int getQuantiaProcessos(const char* nomeArquivo){
    FILE* file = fopen(nomeArquivo, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    int quantiaProcessos = 0;
    char ch;

    while (!feof(file)) {
        ch = fgetc(file);
        if (ch == '\n') {
            quantiaProcessos++;
        }
    }

    fclose(file);
    return quantiaProcessos;
}

Escalonador* adicionarProcessos(const char* nomeArquivo, Escalonador *escalonador) {
    FILE* file = fopen(nomeArquivo, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    int quantiaProcessos = getQuantiaProcessos(nomeArquivo);
    escalonador->numProcessos = quantiaProcessos;
    escalonador->processosExecutados = 0;
    escalonador->tempoSimulacao = 0;

    escalonador->processos = (Processo*)malloc(escalonador->numProcessos * sizeof(Processo));

    int lineNumber = 0;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        sscanf(line, "%d:%d:%d:%d", &escalonador->processos[lineNumber].pid, 
                                    &escalonador->processos[lineNumber].tempoChegada, 
                                    &escalonador->processos[lineNumber].burst, 
                                    &escalonador->processos[lineNumber].prioridadeProcesso);

        escalonador->processos[lineNumber].tempoRestante = escalonador->processos[lineNumber].burst;
        escalonador->processos[lineNumber].tempoEspera = 0;
        escalonador->processos[lineNumber].tempoIO = 0;
        escalonador->processos[lineNumber].tempoTotal = 0;
        strcpy(escalonador->processos[lineNumber].estadoAtual, "new");

        lineNumber++;
    }

    fclose(file);
    return escalonador;
}

Processo* trocarEstado(Processo *processo, char estado[20]) {
    strcpy(processo->estadoAtual, estado);
    return processo;
}

void readArgs(int argc, char **argv) {
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            VERBOSITY = true;
        } else if (strcmp(argv[i], "-F") == 0) {
            MULTIPLEQUEUES = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            FIRSTCOMEFIRSTSERVED = true;
        } else if (strncmp(argv[i], "[a]", 3) == 0) {
            AGING_INTERVAL = atof(argv[i] + 3);
        }
    }

    if (!MULTIPLEQUEUES && !FIRSTCOMEFIRSTSERVED) {
        MULTIPLEQUEUES = true;
    }
}

int osPRNG() {
    return rand();
}

int IOReq() {
    if (osPRNG() % PROB_OF_IO_REQ == 0) {
        return 1;
    } else {
        return 0;
    }
}

int IOTerm(){
    if (osPRNG() % PROB_OF_IO_TERM == 0) {
        return 1;
    } else {
        return 0;
    }
}

void printVerbose(int clock, Processo *processo, int ioReq, int *ioCompleted, int ioCount) {
    printf("%d:%d:%d:%d:", clock, processo->pid, processo->tempoRestante, ioReq);
    if (ioCount > 0) {
        for (int i = 0; i < ioCount; i++) {
            printf("%d", ioCompleted[i]);
            if (i < ioCount - 1) {
                printf(",");
            }
        }
    } else {
        printf("null");
    }
    printf(":%s\n", processo->estadoAtual);
}

void executarEscalonador(Escalonador *escalonador) {
    bool simulacaoAtiva = true;
    Processo *processoAtual = NULL;
    int *ioCompleted = malloc(escalonador->numProcessos * sizeof(int));
    int ioCount;

    while (simulacaoAtiva) {
        simulacaoAtiva = false;

        for (int i = 0; i < escalonador->numProcessos; i++) {
            Processo *processo = &escalonador->processos[i];

            if (processo->tempoChegada <= CLOCK && strcmp(processo->estadoAtual, "new") == 0) {
                trocarEstado(processo, "ready");
            }

            if (strcmp(processo->estadoAtual, "ready") == 0 || strcmp(processo->estadoAtual, "running") == 0 || strcmp(processo->estadoAtual, "waiting") == 0) {
                simulacaoAtiva = true;
            }

            if (strcmp(processo->estadoAtual, "waiting") == 0) {
                if (IOTerm()) {
                    trocarEstado(processo, "ready");
                } else {
                    processo->tempoIO++;
                }
            }
        }

        if (processoAtual != NULL && strcmp(processoAtual->estadoAtual, "running") == 0) {
            if (IOReq()) {
                trocarEstado(processoAtual, "waiting");
            } else {
                processoAtual->tempoRestante--;
            }

            if (processoAtual->tempoRestante <= 0) {
                trocarEstado(processoAtual, "terminated");
                processoAtual->tempoTotal = CLOCK - processoAtual->tempoChegada;
                escalonador->processosExecutados++;
                processoAtual = NULL;
            }
        }

        for (int i = 0; i < escalonador->numProcessos; i++) {
            Processo *processo = &escalonador->processos[i];

            if (strcmp(processo->estadoAtual, "ready") == 0) {
                if (processoAtual == NULL || processo->prioridadeProcesso < processoAtual->prioridadeProcesso) {
                    if (processoAtual != NULL) {
                        trocarEstado(processoAtual, "ready");
                    }
                    processoAtual = processo;
                    trocarEstado(processoAtual, "running");
                }
            }
        }

        if (VERBOSITY) {
            ioCount = 0;
            for (int i = 0; i < escalonador->numProcessos; i++) {
                if (strcmp(escalonador->processos[i].estadoAtual, "waiting") == 0) {
                    if (IOTerm()) {
                        ioCompleted[ioCount++] = escalonador->processos[i].pid;
                        trocarEstado(&escalonador->processos[i], "ready");
                    }
                }
            }
            if (processoAtual != NULL) {
                printVerbose(CLOCK, processoAtual, IOReq(), ioCompleted, ioCount);
            }
        }

        CLOCK++;
    }

    free(ioCompleted);
}

void printStatistics(Escalonador *escalonador) {
    printf("=========+=================+=================+============\n");
    printf("Processo | Tempo total     | Tempo total     | Tempo total\n");
    printf("         | em estado Ready | em estado Wait  | no sistema \n");
    printf("=========+=================+=================+============\n");

    int totalTempoSistema = 0;
    int menorTempo = __INT_MAX__;
    int maiorTempo = 0;

    for (int i = 0; i < escalonador->numProcessos; i++) {
        Processo *processo = &escalonador->processos[i];
        if (processo->tempoTotal > 0) {
            printf("%d       | %d               | %d               | %d\n", 
                   processo->pid, 
                   processo->tempoEspera, 
                   processo->tempoIO, 
                   processo->tempoTotal);

            totalTempoSistema += processo->tempoTotal;
            if (processo->tempoTotal < menorTempo) {
                menorTempo = processo->tempoTotal;
            }
            if (processo->tempoTotal > maiorTempo) {
                maiorTempo = processo->tempoTotal;
            }
        }
    }

    printf("=========+=================+=================+============\n");
    printf("Tempo total de simulação.: %d\n", CLOCK);
    printf("Número de processos......: %d\n", escalonador->numProcessos);
    printf("Menor tempo de execução..: %d\n", menorTempo);
    printf("Maior tempo de execução..: %d\n", maiorTempo);
    printf("Tempo médio de execução..: %.2f\n", totalTempoSistema / (float) escalonador->numProcessos);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <nome do arquivo> [opções]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Escalonador escalonador;
    adicionarProcessos(argv[1], &escalonador);

    readArgs(argc, argv);

    executarEscalonador(&escalonador);

    printStatistics(&escalonador);

    liberarMemoria(&escalonador);

    return 0;
}
