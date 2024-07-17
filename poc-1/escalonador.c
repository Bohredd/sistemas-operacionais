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

        // printf("PID: %d CHEGADA: %d QUANTIA: %d PRIORIDADE: %d\n",
        //        escalonador->processos[lineNumber].pid,
        //        escalonador->processos[lineNumber].tempoChegada,
        //        escalonador->processos[lineNumber].quantiaClocksNecessaria,
        //        escalonador->processos[lineNumber].prioridadeProcesso);
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

//  IMPLEMENTAR AS ESTATISTICAS DO PROCESSO
void MultipleQueues(Escalonador *escalonador, Historico *historico) {
    printf("Executando escalonador com Múltiplas Filas com Prioridade\n");

    escalonador = reordenarEscalonador(escalonador, historico);

    bool executandoProcessos = true;
    int index = 0;
    bool trocado = false;
    PROCESSOSSOLICITADOS = escalonador->numProcessos;

    // printf("Numero de processos: %d\n", escalonador->numProcessos);
    while (executandoProcessos) {
        // printf("CLOCK ATUAL %d\n", CLOCK);

        Processo *processoAtual = &escalonador->processos[index];

        if (chegouProcesso(escalonador, CLOCK)) {
            Processo novoProcesso = getProcessoChegada(escalonador, CLOCK);
            // printf("Novo processo de PID %d chegou no clock %d\n", novoProcesso.pid, CLOCK);
            // printf("Processo atual de PID %d\n", processoAtual->pid);
            novoProcesso.tempoQueFicouReady++;

            int indexNovo = encontrarIndicePorPID(escalonador, novoProcesso.pid);
            if (processoAtual->clocksFaltantes != 0) {

                Processo processoAtual = escalonador->processos[0];
                processoAtual.tempoQueFicouWait++;
                escalonador->processos[0] = novoProcesso;
                escalonador->processos[indexNovo] = processoAtual;
                processoAtual = escalonador->processos[0];
                trocado = true;
            } 

        } else {
            // printf("Não chegou novo processo e o atual tem clocks faltantes %d pid %d \n", processoAtual->clocksFaltantes, processoAtual->pid);
            if(! processoAtual->clocksFaltantes > 0){
                processoAtual = &escalonador->processos[0];
                escalonador = reordenarEscalonador(escalonador, historico);
            }
        }

        if(processoAtual->tempoChegada > CLOCK) {
            printf("");
        } else {
            processoAtual->clocksFaltantes--;
        }
        if (processoAtual->clocksFaltantes == 0) {
            // printf("Processo terminado: %d\n", processoAtual->pid);
            processoAtual->terminado = true;
            processoAtual->tempoQueFicouNoEscalonador = CLOCK - processoAtual->tempoChegada;

            historico->processos[historico->numProcessos++] = *processoAtual;

            NUMPROCESSOSEXECUTADOS++;
            moverProcessosTerminados(escalonador, historico);

            if (!trocado) {
                escalonador = reordenarEscalonador(escalonador, historico);
            } 
            trocado = false;
            // printf("Numero de processos: %d\n", PROCESSOSSOLICITADOS);
            // printf("Numero de processos executados: %d\n", NUMPROCESSOSEXECUTADOS);

            if (NUMPROCESSOSEXECUTADOS < PROCESSOSSOLICITADOS) {
                index = 0;
            } else {
                executandoProcessos = false;
            }
        }
        CLOCK++;
        // printf("\n");
    }
}

void ShortestJobFirst(Escalonador *escalonador, Historico *historico) {
    // arruamr essa porra aqui
    printf("Executando escalonador com Shortest Job First preemptivo (SJF)\n");

    escalonador = reordenarEscalonador(escalonador, historico);

    bool executandoProcessos = true;
    int index = 0;
    float a = AGGINGPADRAO;

    printf("Número de processos: %d\n", escalonador->numProcessos);
    while(executandoProcessos) {
        if(NUMPROCESSOSEXECUTADOS == escalonador->numProcessos) {
            executandoProcessos = false;
        }

        if(chegouProcesso(escalonador, CLOCK)) {
            Processo processo = getProcessoChegada(escalonador, CLOCK);
            printf("Processo de PID %d chegou no clock %d\n", processo.pid, CLOCK);
        }
        if(index < escalonador->numProcessos) {
            float ta = escalonador->processos[index].clocksFaltantes;
            float Ta = escalonador->processos[index].tempoChegada;
            float Tn = (a * ta) + ((1 - a) * Ta);
            escalonador->processos[index].clocksFaltantes = (int) Tn; // Atualiza a estimativa
        }

        printf("Executando processo de PID: %d Prioridade: %d Chegada: %d Clocks Faltantes: %d \n",
               escalonador->processos[index].pid, escalonador->processos[index].prioridadeProcesso,
               escalonador->processos[index].tempoChegada, escalonador->processos[index].clocksFaltantes);

        index++;
        CLOCK++;
        NUMPROCESSOSEXECUTADOS++;
    }
}

void executarEscalonador(Escalonador *escalonador, Historico *historico, int argc, char **argv) {
    if (MULTIPLEQUEUES) {
        MultipleQueues(escalonador, historico);
    } else if (SHORTESTJOBFIRST) {
        ShortestJobFirst(escalonador, historico);
    } else {
        MultipleQueues(escalonador, historico);
    }

    if (VERBOSITY) {
        printf("Executando escalonador com verbosity\n");
    }
}

Historico* arrumarHistorico(Historico *historico) {
    if (historico == NULL || historico->numProcessos <= 0 || historico->processos == NULL) {
        return historico;
    }

    Historico *novoHistorico = (Historico *) malloc(sizeof(Historico));
    if (novoHistorico == NULL) {
        printf("Erro ao alocar memória!\n");
        return historico;
    }
    
    novoHistorico->numProcessos = historico->numProcessos;

    novoHistorico->processos = (Processo *) malloc(novoHistorico->numProcessos * sizeof(Processo));
    if (novoHistorico->processos == NULL) {
        printf("Erro ao alocar memória!\n");
        free(novoHistorico);
        return historico;
    }

    int novoTamanho = 0;
    int i, j;

    for (i = 0; i < historico->numProcessos; i++) {
        int pidAtual = historico->processos[i].pid;
        for (j = 0; j < novoTamanho; j++) {
            if (pidAtual == novoHistorico->processos[j].pid) {
                novoHistorico->processos[j] = historico->processos[i];
                break;
            }
        }

        if (j == novoTamanho) {
            novoHistorico->processos[novoTamanho] = historico->processos[i];
            novoTamanho++;
        }
    }

    free(historico->processos);
    novoHistorico->numProcessos = novoTamanho;
    return novoHistorico;
}

void mostrarEstaticasProcessos(Historico *historico){

    historico = arrumarHistorico(historico);

    printf("=========+=================+=================+============\n");
    printf("Processo | Tempo total em  | Tempo total em  | Tempo total no\n");
    printf("         | estado Ready    | estado Wait     | sistema\n");
    printf("=========+=================+=================+============\n");
    
    for(int i = 0; i < historico->numProcessos; i++) {
        printf(" %d       | %d               | %d               | %d\n",
               historico->processos[i].pid,
               historico->processos[i].tempoQueFicouReady,
               historico->processos[i].tempoQueFicouWait,
               historico->processos[i].tempoQueFicouNoEscalonador);
    }
    
    printf("=========+=================+=================+============\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo de processos> [-V] [-F] [-S] [a]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Escalonador escalonador;
    Historico historico = {0, malloc(100 * sizeof(Processo))};

    escalonador = adicionarProcessos(argv[1], &escalonador);
    readArgs(argc, argv);
    executarEscalonador(&escalonador, &historico, argc, argv);

    mostrarEstaticasProcessos(&historico);

    liberarMemoria(&escalonador);
    liberarHistorico(&historico);

    return EXIT_SUCCESS;
}
