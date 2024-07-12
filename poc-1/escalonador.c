#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

bool VERBOSITY = false;
bool MULTIPLEQUEUES = false;
bool SHORTESTJOBFIRST = false;

float AGGINGINTERVAL[2] = {0.0, 0.0};

int CLOCK = 0;
int NUMPROCESSOSEXECUTADOS = 0;

typedef struct Processo {
    int pid;
    int tempoChegada;
    int quantiaClocksNecessaria;
    int clocksFaltantes;
    int prioridadeProcesso;
    char estadoAtual[20];
    bool terminado;
} Processo;

typedef struct Escalonador {
    int numProcessos;
    Processo *processos;
} Escalonador;

void liberarMemoria(Escalonador *escalonador) {
    free(escalonador->processos);
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

Escalonador* reordenarEscalonador(Escalonador *escalonador) {
    if (MULTIPLEQUEUES) {
        bool trocado;
        do {
            trocado = false;
            for (int i = 0; i < escalonador->numProcessos - 1; i++) {
                if (escalonador->processos[i].prioridadeProcesso < escalonador->processos[i + 1].prioridadeProcesso) {
                    Processo temp = escalonador->processos[i];
                    escalonador->processos[i] = escalonador->processos[i + 1];
                    escalonador->processos[i + 1] = temp;
                    trocado = true;
                }
            }
        } while (trocado);
    } else {
        bool trocado;
        do {
            trocado = false;
            for (int i = 0; i < escalonador->numProcessos - 1; i++) {
                if (escalonador->processos[i].tempoChegada > escalonador->processos[i + 1].tempoChegada) {
                    Processo temp = escalonador->processos[i];
                    escalonador->processos[i] = escalonador->processos[i + 1];
                    escalonador->processos[i + 1] = temp;
                    trocado = true;
                }
            }
        } while (trocado);
    }

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

void MultipleQueues(Escalonador *escalonador) {
    printf("Executando escalonador com Múltiplas Filas com Prioridade\n");

    escalonador = reordenarEscalonador(escalonador);

    bool executandoProcessos = true;
    int index = 0;

    printf("Numero de processos: %d\n", escalonador->numProcessos);
    while(executandoProcessos) {

        if(NUMPROCESSOSEXECUTADOS == (escalonador->numProcessos -1)) {
            executandoProcessos = false;
        }

        if(chegouProcesso(escalonador, CLOCK)) {
            Processo processo = getProcessoChegada(escalonador, CLOCK);
            printf("Processo de PID %d chegou no clock %d\n", processo.pid, CLOCK);
        }

        printf("Executando processo de PID: %d Prioridade: %d e Chegada: %d Clocks Faltantes: %d \n", escalonador->processos[index].pid, escalonador->processos[index].prioridadeProcesso, escalonador->processos[index].tempoChegada, escalonador->processos[index].clocksFaltantes);
        index++;
        CLOCK++;
        NUMPROCESSOSEXECUTADOS++;
    }
}

void ShortestJobFirst(Escalonador *escalonador) {
    printf("Executando escalonador com Shortest Job First preemptivo (SJF)\n");

    escalonador = reordenarEscalonador(escalonador);

    bool executandoProcessos = true;
    int index = 0;

    printf("Numero de processos: %d\n", escalonador->numProcessos);
    while(executandoProcessos) {

        if(NUMPROCESSOSEXECUTADOS == (escalonador->numProcessos -1)) {
            executandoProcessos = false;
        }

        if(chegouProcesso(escalonador, CLOCK)) {
            Processo processo = getProcessoChegada(escalonador, CLOCK);
            printf("Processo de PID %d chegou no clock %d\n", processo.pid, CLOCK);
        }

        printf("Executando processo de PID: %d Prioridade: %d e Chegada: %d Clocks Faltantes: %d \n", escalonador->processos[index].pid, escalonador->processos[index].prioridadeProcesso, escalonador->processos[index].tempoChegada, escalonador->processos[index].clocksFaltantes);
        index++;
        CLOCK++;
        NUMPROCESSOSEXECUTADOS++;
    }
}


void executarEscalonador(Escalonador *escalonador, int argc, char **argv) {
    if (MULTIPLEQUEUES) {
        MultipleQueues(escalonador);
    } else if (SHORTESTJOBFIRST) {
        ShortestJobFirst(escalonador);
    } else {
        MultipleQueues(escalonador);
    }

    if (VERBOSITY) {
        printf("Executando escalonador com verbosity\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo de processos> [-V] [-F] [-S] [a]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Escalonador escalonador;
    escalonador = adicionarProcessos(argv[1], &escalonador);
    readArgs(argc, argv);
    executarEscalonador(&escalonador, argc, argv);
    liberarMemoria(&escalonador);

    return EXIT_SUCCESS;
}
