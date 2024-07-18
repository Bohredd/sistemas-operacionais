#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

bool VERBOSITY = false;
bool MULTIPLEQUEUES = false;
bool SHORTESTJOBFIRST = false;

float AGGINGINTERVAL[2] = {0.0, 0.0};
float AGGINGPADRAO = 0.5;

int CLOCK = 0;
int NUMPROCESSOSEXECUTADOS = 0;
int PROCESSOSSOLICITADOS = 0;

typedef struct Processo {
    int pid;
    int tempoChegada;
    int quantiaClocksNecessaria;
    int clocksFaltantes;
    int prioridadeProcesso;
    char estadoAtual[20];
    bool terminado;
    int tempoQueFicouWait;
    int tempoQueFicouReady;
    int tempoQueFicouNoEscalonador;
    int tempoDeExecucaoNecessario;
    bool requisicaoIO;
    bool completouIO;
} Processo;

typedef struct Escalonador {
    int numProcessos;
    Processo *processos;
} Escalonador;

typedef struct Historico {
    int numProcessos;
    Processo *processos;
} Historico;

void liberarMemoria(Escalonador *escalonador) {
    free(escalonador->processos);
}

void liberarHistorico(Historico *historico) {
    free(historico->processos);
}

int getQuantiaProcessos(const char *nomeArquivo) {
    FILE *file = fopen(nomeArquivo, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    int quantiaProcessos = 0;
    char ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            quantiaProcessos++;
        }
    }

    fclose(file);
    return quantiaProcessos;
}

Escalonador adicionarProcessos(const char *nomeArquivo, Escalonador *escalonador) {
    FILE *file = fopen(nomeArquivo, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    int quantiaProcessos = getQuantiaProcessos(nomeArquivo);

    escalonador->numProcessos = quantiaProcessos;
    escalonador->processos = (Processo *)malloc(escalonador->numProcessos * sizeof(Processo));
    if (escalonador->processos == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int lineNumber = 0;

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%d:%d:%d:%d", 
               &escalonador->processos[lineNumber].pid,
               &escalonador->processos[lineNumber].tempoChegada,
               &escalonador->processos[lineNumber].quantiaClocksNecessaria,
               &escalonador->processos[lineNumber].prioridadeProcesso);

        escalonador->processos[lineNumber].clocksFaltantes = escalonador->processos[lineNumber].quantiaClocksNecessaria;
        strcpy(escalonador->processos[lineNumber].estadoAtual, "ready");
        escalonador->processos[lineNumber].terminado = false;
        escalonador->processos[lineNumber].tempoQueFicouWait = 0;
        escalonador->processos[lineNumber].tempoQueFicouReady = 0;
        escalonador->processos[lineNumber].tempoQueFicouNoEscalonador = 0;
        escalonador->processos[lineNumber].tempoDeExecucaoNecessario = escalonador->processos[lineNumber].quantiaClocksNecessaria;
        escalonador->processos[lineNumber].requisicaoIO = false;
        escalonador->processos[lineNumber].completouIO = false;

        lineNumber++;
    }

    fclose(file);

    return *escalonador;
}

void readArgs(int argc, char **argv) {
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-V") == 0) {
            VERBOSITY = true;
        } else if (strcmp(argv[i], "-F") == 0) {
            MULTIPLEQUEUES = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            SHORTESTJOBFIRST = true;
        } else if (strcmp(argv[i], "[a]") == 0) {
            SHORTESTJOBFIRST = false;
            AGGINGINTERVAL[0] = 0.0;
            AGGINGINTERVAL[1] = 1.0;
        }
    }

    if (!MULTIPLEQUEUES && !SHORTESTJOBFIRST) {
        MULTIPLEQUEUES = true;
    }
}

void moverProcessosTerminados(Escalonador *escalonador, Historico *historico) {
    int j = 0;
    for (int i = 0; i < escalonador->numProcessos; i++) {
        if (escalonador->processos[i].terminado || escalonador->processos[i].clocksFaltantes == 0) {
            historico->processos[historico->numProcessos++] = escalonador->processos[i];
        } else {
            escalonador->processos[j++] = escalonador->processos[i];
        }
    }
    escalonador->numProcessos = j;
}

Escalonador* reordenarEscalonador(Escalonador *escalonador, Historico *historico) {
    moverProcessosTerminados(escalonador, historico);

    bool trocado;
    do {
        trocado = false;
        for (int i = 0; i < escalonador->numProcessos - 1; i++) {
            if (MULTIPLEQUEUES && escalonador->processos[i].prioridadeProcesso < escalonador->processos[i + 1].prioridadeProcesso) {
                Processo temp = escalonador->processos[i];
                escalonador->processos[i] = escalonador->processos[i + 1];
                escalonador->processos[i + 1] = temp;
                trocado = true;
            } else if (!MULTIPLEQUEUES && escalonador->processos[i].tempoChegada > escalonador->processos[i + 1].tempoChegada) {
                Processo temp = escalonador->processos[i];
                escalonador->processos[i] = escalonador->processos[i + 1];
                escalonador->processos[i + 1] = temp;
                trocado = true;
            }
        }
    } while (trocado);

    return escalonador;
}

Processo getProcessoChegada(Escalonador *escalonador, int clock) {
    for (int i = 0; i < escalonador->numProcessos; i++) {
        if (escalonador->processos[i].tempoChegada == clock) {
            return escalonador->processos[i];
        }
    }
    Processo nullProcesso = { .pid = -1 };
    return nullProcesso;
}

bool chegouProcesso(Escalonador *escalonador, int clock) {
    for (int i = 0; i < escalonador->numProcessos; i++) {
        if (escalonador->processos[i].tempoChegada == clock) {
            return true;
        }
    }
    return false;
}

int encontrarIndicePorPID(Escalonador *escalonador, int pid) {
    for (int i = 0; i < escalonador->numProcessos; i++) {
        if (escalonador->processos[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

void printVerbose(Processo *processo, const char *estado) {
    if (VERBOSITY) {
        fprintf(stderr, "%d:%d:%d:%d:%s:%s\n",
                CLOCK,
                processo->pid,
                processo->clocksFaltantes,
                processo->requisicaoIO,
                processo->completouIO ? "0" : "null",
                estado);
    }
}

void MultipleQueues(Escalonador *escalonador, Historico *historico) {
    printf("Executando escalonador com Múltiplas Filas com Prioridade\n");

    escalonador = reordenarEscalonador(escalonador, historico);

    bool executandoProcessos = true;
    int index = 0;
    bool trocado = false;
    PROCESSOSSOLICITADOS = escalonador->numProcessos;

    while (executandoProcessos) {
        Processo *processoAtual = &escalonador->processos[index];
        strcpy(processoAtual->estadoAtual, "running");

        if (chegouProcesso(escalonador, CLOCK)) {
            Processo novoProcesso = getProcessoChegada(escalonador, CLOCK);
            if (novoProcesso.pid != -1) {
                novoProcesso.tempoQueFicouReady++;
                escalonador->processos[index] = novoProcesso;
                escalonador->processos[0] = *processoAtual;
                strcpy(processoAtual->estadoAtual, "ready");
                trocado = true;
            }
        }

        processoAtual->tempoQueFicouReady++;
        processoAtual->clocksFaltantes--;

        if (VERBOSITY) {
            const char *estado = processoAtual->clocksFaltantes == 0 ? "end" :
                                 trocado ? "preempted" : "running";
            printVerbose(processoAtual, estado);
        }

        if (processoAtual->clocksFaltantes == 0) {
            processoAtual->terminado = true;
            strcpy(processoAtual->estadoAtual, "terminated");
            NUMPROCESSOSEXECUTADOS++;
        }

        if (NUMPROCESSOSEXECUTADOS == PROCESSOSSOLICITADOS) {
            executandoProcessos = false;
            printf("Todos os processos foram finalizados.\n");
        }

        if (index < escalonador->numProcessos - 1 && trocado) {
            index++;
        } else {
            index = 0;
        }

        if (CLOCK % 5 == 0) {
            escalonador = reordenarEscalonador(escalonador, historico);
        }

        CLOCK++;
    }
}

Processo getShortestJobFirst(Escalonador *escalonador) {
    int min = INT_MAX;
    int index = -1;
    for (int i = 0; i < escalonador->numProcessos; i++) {
        if (!escalonador->processos[i].terminado && escalonador->processos[i].clocksFaltantes < min) {
            min = escalonador->processos[i].clocksFaltantes;
            index = i;
        }
    }
    return escalonador->processos[index];
}

void ShortestJobFirst(Escalonador *escalonador, Historico *historico) {
    printf("Executando escalonador com Shortest Job First Preemptivo\n");

    escalonador = reordenarEscalonador(escalonador, historico);

    bool executandoProcessos = true;
    PROCESSOSSOLICITADOS = escalonador->numProcessos;

    while (executandoProcessos) {
        Processo *processoAtual = &escalonador->processos[0];
        strcpy(processoAtual->estadoAtual, "running");

        if (chegouProcesso(escalonador, CLOCK)) {
            Processo novoProcesso = getProcessoChegada(escalonador, CLOCK);
            novoProcesso.tempoQueFicouReady++;
            escalonador->processos[0] = novoProcesso;
            int indexNovo = encontrarIndicePorPID(escalonador, novoProcesso.pid);
            escalonador->processos[indexNovo] = *processoAtual;
            strcpy(processoAtual->estadoAtual, "ready");
        }

        Processo shortestJob = getShortestJobFirst(escalonador);
        if (shortestJob.pid != processoAtual->pid) {
            strcpy(processoAtual->estadoAtual, "ready");
            processoAtual = &shortestJob;
            strcpy(processoAtual->estadoAtual, "running");
        }

        processoAtual->clocksFaltantes--;

        if (VERBOSITY) {
            const char *estado = processoAtual->clocksFaltantes == 0 ? "end" :
                                 strcmp(processoAtual->estadoAtual, "ready") == 0 ? "preempted" : "running";
            printVerbose(processoAtual, estado);
        }

        if (processoAtual->clocksFaltantes == 0) {
            processoAtual->terminado = true;
            strcpy(processoAtual->estadoAtual, "terminated");
            NUMPROCESSOSEXECUTADOS++;
        }

        if (NUMPROCESSOSEXECUTADOS == PROCESSOSSOLICITADOS) {
            executandoProcessos = false;
            printf("Todos os processos foram finalizados.\n");
        }

        CLOCK++;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s <arquivo de processos> [-V] [-F | -S] [a]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Escalonador escalonador;
    Historico historico;
    historico.numProcessos = 0;
    historico.processos = (Processo *)malloc(100 * sizeof(Processo));

    readArgs(argc, argv);
    escalonador = adicionarProcessos(argv[1], &escalonador);

    if (MULTIPLEQUEUES) {
        MultipleQueues(&escalonador, &historico);
    } else if (SHORTESTJOBFIRST) {
        ShortestJobFirst(&escalonador, &historico);
    }

    liberarMemoria(&escalonador);
    liberarHistorico(&historico);

    return EXIT_SUCCESS;
}
