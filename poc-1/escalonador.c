#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Processo {
    int pid;
    int tempoChegada;
    int quantiaClocksNecessaria;
    int prioridadeProcesso;
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

void executarEscalonador(Escalonador *escalonador, int argc, char **argv) {

    int i;
    printf("Número de argumentos: %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("ARGV[%d]: %s\n", i, argv[i]);
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

    executarEscalonador(&escalonador, argc, argv);

    liberarMemoria(escalonador);

    return 0;
}
