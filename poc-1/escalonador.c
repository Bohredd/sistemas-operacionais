#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool VERBOSITY = false;
bool MULTIPLEQUEUES = false;
bool SHORTESTJOBFIRST = false;

float aggingInterval[2] = {0.0, 0.0};

typedef struct Processo {
    int pid;
    int tempoChegada;
    int quantiaClocksNecessaria;
    int prioridadeProcesso;
    char estadoAtual[20];
} Processo;

typedef struct Escalonador {
    int numProcessos;
    Processo *processos;
} Escalonador;

void liberarMemoria(Escalonador escalonador){
    free(escalonador.processos);
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

Escalonador adicionarProcessos(const char* nomeArquivo, Escalonador* escalonador) {
    FILE* file = fopen(nomeArquivo, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    int quantiaProcessos = getQuantiaProcessos(nomeArquivo);

    escalonador->numProcessos = quantiaProcessos;

    int lineNumber = 1;
    char line[256];

    escalonador->processos = (Processo*)malloc(escalonador->numProcessos * sizeof(Processo));

    int qualInformacao = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        char *token = strtok(line, ":");

        while (token != NULL) {

            if (qualInformacao == 0) {
                escalonador->processos[lineNumber - 1].pid = atoi(token);
            } else if (qualInformacao == 1){
                escalonador->processos[lineNumber - 1].tempoChegada = atoi(token);
            } else if (qualInformacao == 2) {
                escalonador->processos[lineNumber - 1].quantiaClocksNecessaria = atoi(token);
            } else if (qualInformacao == 3){
                escalonador->processos[lineNumber - 1].prioridadeProcesso = atoi(token);
            } else {
                continue;
            }
            token = strtok(NULL, ":");
            qualInformacao++;
        }

        qualInformacao = 0;

        lineNumber++;
    }

    fclose(file);

    return *escalonador;
}

Processo* trocarEstado(Processo *processo, char estado[20]) {
    free(processo->estadoAtual);
    strcpy(processo->estadoAtual, estado);
    return processo;
}

void readArgs(int argc, char **argv) {
    int i;
    for (i = 2; i < argc; i++) {

        if(strcmp(argv[i], "-V") == 0) {
            VERBOSITY = true;
        } else if (strcmp(argv[i], "-F") == 0) {
            MULTIPLEQUEUES = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            SHORTESTJOBFIRST = true;
        } else if (strcmp(argv[i], "[a]") == 0) {
            SHORTESTJOBFIRST = false;
            aggingInterval[0] = 0.0;
            aggingInterval[1] = 1.0;
        }
    }

    if (!MULTIPLEQUEUES && !SHORTESTJOBFIRST) {
        MULTIPLEQUEUES = true;
    }
}

void executarEscalonador(Escalonador *escalonador, int argc, char **argv) {

    if (MULTIPLEQUEUES){
        printf("executando escalonador com multiplas filas\n");
    } else if (SHORTESTJOBFIRST) {
        printf("executando escalonador com shortest job first\n");
    } else {
        printf("executando escalonador com multiplas filas\n");
    }

    if (VERBOSITY) {
        printf("executando escalonador com verbosity\n");
    }
}

int main(int argc, char **argv[]){

    Escalonador escalonador;

    escalonador = adicionarProcessos(argv[1], &escalonador);

    for (int i = 0; i < escalonador.numProcessos; i++) {
        printf("PID: %d\n", escalonador.processos[i].pid);
        printf("Tempo de execucao: %d\n", escalonador.processos[i].tempoChegada);
        printf("Quantia de clocks necessaria: %d\n", escalonador.processos[i].quantiaClocksNecessaria);
        printf("Prioridade do processo: %d\n", escalonador.processos[i].prioridadeProcesso);
    }

    readArgs(argc, argv);

    executarEscalonador(&escalonador, argc, argv);

    liberarMemoria(escalonador);

    return 0;
}
