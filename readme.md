O Supervisor - um simulador de escalonadores de CPU

Objetivo
Solidificar seus conhecimentos sobre escalonamento e seus impactos em um sistema.

Metodologia
Você irá projetar e implementar um simulador de propósito geral - o Supervisor - de um Sistema Operacional simples.

Para avaliar a utilidade de seu Supervisor e ter uma noção de políticas efetivas de escalonamento, você irá comparar ao menos 2 políticas: Filas Múltiplas com Prioridade (FMP) e Shortest Job First preemptivo (SJF).


Visão Geral:

Você irá implementar um supervisor de sistema (simulador) capaz de gerenciar processos, dispositivos e escalonadores. Isto acontece dentro de um loop de código geral.

Um processo é um cliente de serviços, é ele quem necessita utilizar os recursos do sistema durante sua execução.

Um dispositivo representa um recurso do sistema. Nesta simulação, os dispositivos disponíveis para um processo são a CPU e o disco. Há também um relógio (clock) e um pseudo-dispositivo que causa interrupção sempre que um novo processo entra no sistema.

Um escalonador coordena acessos a um dispositivo. Ele enfileira processos que estão esperando o dispositivo e escolhe qual deles deve ser o próximo a usá-lo quando livre.

De modo geral, o Supervisor funciona da seguinte maneira: processos chegam no dispositivo de chegada de processo e entram no sistema. Durante sua estada, um processo tipicamente alterna períodos de uso da CPU (chamado de burst) e períodos de Entrada/Saída (I/O).

O Loop principal do Supervisor é responsável pela movimentação dos processos pelo sistema. Nele os processos são enviados ao escalonador, o próximo processo a ser executado é escolhido, e processos que solicitaram I/O são iniciados ou parados.

O escalonador de disco e o escalonador de CPU decidem qual processo deve ser o próximo a receber o dispositivo respectivo. O clock é usado para permitir preempção.

Estrutura de código:

Seu trabalho é implementar um sistema simplificado de gerência de processos usando diferentes técnicas de escalonamento.

O Supervidor trabalha em um loop (while()) verificando eventos. A cada iteração o clock é incrementado (um loop == um ciclo de Clock/Clock Tick).

Ao menos sete coisas necessitam ser feitas pelo loop do Supervisor a cada ciclo, a maioria de acordo com o diagrama de estados de um processoem um SO, como na figura abaixo. Lembre-se que a preempção acontece ao FIM de um quantum/fatia de tempo.



O loop principal de seu Supervisor deverá lembrar o pseudocódigo abaixo (irá variar dependendo da política de escalonamento).

Início
1. se há novos processos, colocar no estado Ready
2. se o processo executando na CPU (Running) usou seu quantum de tempo, colocar no estado Ready
3. se o processo executando na CPU requisitou I/O, colocar no estado Wait
4. se a CPU está vaga, colocar outro processo no estado Running
5. para todos os processos esperando por I/O, verificar se a requisição foi satisfeita
6. se o I/O de um processo terminou, colocar o processo em estado Ready
7. atualizar estatísticas de execução
Fim

Você precisa manter atualizados os seguintes dados estatísticos para os processos:
número de processos executados
tempo total decorrido
tempo total por processo (throughput)
tempo total que cada processo ficou em cada estado (Ready, Wait, Running)
processos com maior e menor tempo no sistema

Como seus processos não executarão tarefas/código real, temos um pequeno problema: como eles solicitarão I/O?
Solução: quando um processo está em estado Running, você terá uma função que aleatoriamente decide se ele requisitou I/O. Para os processos em estado Wait, haverá uma função similar que decide aleatoriamente se o I/O completou ou não. PAra os processos em Wait, esta função será aplicada em ordem FCFS (First-come First-served).

Abaixo está o código para estas funções.

#include "osPRNG.h"
#define PROB_OF_IO_REQ 10
#define PROB_OF_IO_TERM 4
int IOReq() {
if( osPRNG() % PROB_OF_IO_REQ == 0 )
return 1;
else
return 0;
}

int IOTerm(){
if( osPRNG() % PROB_OF_IO_TERM == 0 )
return 1;
else
return 0;
}


A função osPRNG() está no arquivo anexo. Não utilize as funções rand/random da linguagem C.

Quando não houver mais nenhum processo em nenhum estado válido (Running, Wait, Ready), sua simulação deve terminar.


Implementação do Supervisor:
O Supervisor nunca deve estar parado se há um processo ao menos. Quando processos são completados, seu escalonador irá atualizar as estatísticas de execução e escalonar o próximo processo. Se um processo volta ao estado Ready, deve ser colocado no local apropriado a sua prioridade. Para cada ciclo de clock em que um processo está em estado Running, o tempo restante de execução deve decrescer em 1. Sua função main ficará (mais ou menos) na forma abaixo:

#include <stdlib.h>

int main(int argc, char **argv){
/* inicializar clock */
clock=0;

/* adicionar novos processos ao estado Ready */
/* (seu código aqui) */

clock++;
/* Loop principal do Supervisor */
while( /* há processos no sistema */ ) {
currentProcess = /* escolher processo a executar */;
while( 1 ) { /* loop principal de gerência de processos */
/* se houver, adicionar novos processos ao estado Ready */

/* enquanto houver processo(s) em estado Wait (I/O) */
/* verificar se o I/O completou para cada processo em espera (ordem FIFO) */
status = IOTerm();
if (status == 1){
/* colocar o processo que completou em estado Ready */
}

/* se o processo terminou sua execução (tempo restante 1) */
/* marcar o processo atual como swappedOut (termino de execução) */

/* se for um escalonador preemptivo e há processo com */
/* prioridade superior em estado Ready */
/* marcar o processo atual como swappedOut (preempção) */

/* verificar se o processo atual requisita I/O */
if ( /* processo atual não está completo */)
status = IOReq();
if (status == 1){
/* need to do I/O */
/* marcar o processo atual como swappedOut (wait on I/O) */
} else {
/* marcar o processo atual como swappedOut (fim do quantum) */
}

/* atualizar estatísticas de execução */

clock++;

/* se o processo atual foi swappedOut */
/* mover o processo para a fila adequada e sair deste laço */
} /* fim do loop de gerência de processos */

} /* fim do loop principal do Supervisor */

Observações: "/* escolher processo a executar */" irá depender de sua função de escalonamento (se o loop de gerência de processos for bem executado por vocÊ, não deveria sofrer mudanças se o algoritmo de escalonamento for alterado).
Pode ser que não haja processo em execução em um ciclo de clock (ex. todos em Wait). Neste caso, use um processo "idle" (nenhuma estatística precisa ser coletada para este pseudoprocesso).
Quaisquer empates (ex. dois processos com a mesma prioridade chegam ao mesmo tempo no escalonador) devem ser resolvidos pelo PID (menor PID ganha sempre).


Entrada e saída do simulador:
Seu Supervisor receberá dados através da entrada padrão (stdin).
Esta entrada especificará os processos a serem escalonados e a saída do Supervisor serão as estatísticas produzidas.
Exemplo:
% supervisor < inputfile.txt
=========+=================+=================+============
Processo | Tempo total     | Tempo total     | Tempo total
         | em estado Ready | em estado Wait  | no sistema
=========+=================+=================+============
 0       | 0               | 0               | 2
 1       | 0               | 0               | 2
=========+=================+=================+============

Tempo total de simulação.: 5
Número de processos......: 2
Menor tempo de execução..: 2
Maior tempo de execução..: 2
Tempo médio de execução..: 2
Tempo médio em Ready/Wait: 0

% /*fim do exemplo*/

Formato de entrada:
A entrada de dados consiste em arquivo de texto formatado em quatro colunas.
Cada linha tem o seguinte formato:
Process Id (PID) : Tempo de chegada : Burst : Prioridade

Observação: Mesmo que não seja usada, a prioridade deve conter um valor.

Por exemplo, o inputfile.txt do exemplo acima seria:
123:0:2:1
124:1:2:0

Este arquivo contém dois processos. O primeiro com PID 123, tempo de chegada 0 (início da simulação), precisa executar 2 clock ticks de trabalho e tem prioridade 1. O segundo processo tem PID 124, chega no clock 1, precisa de 2 clock ticks e tem uma prioridade inicial 0.

Os PIDs serão sempre únicos. O tempo de chegada será zero ou maior. O tempo de burst será sempre maior que zero. A prioridade será entre zero e 7 (inclusive). Quando usado, um valor menor de prioridade indica prioridade maior.

Saída de dados:
Sua saída deverá seguir o formato do exemplo dado acima (tabela de tempos por processo e ao final estatísticas gerais da simulação).


Saída "Verbose":
Seu programa deve, além da saída descrita acima, permitir um modo "verbose" se a opção -v for usada na linha de comando. Neste modo, a cada clock tick uma linha deve ser escrita na saída de erro (stderr). Esta linha deve ter o seguinte formato:
<clock tick>:<pid>:<tempo restante>:<IOReq>:<IOs completados>:<estado>

Onde cada valor é:
<clock tick>: valor atual do clock tick
<pid>: identificador do processo atual
<tempo restante>: tempo restante ao processo atual
<IOReq>: true ou false (0 ou 1) se houve requisição de IO
<IOs completados>: lista de pids com requisições de IO que completaram neste ciclo, NULL se nenhum completou
<estado>: estado do processo ao fim do loop (preempted, running, wait, idle, end)

Por exemplo, o arquivo inputfile.txt abaixo poderia produzir a seguinte saída verbose:
inputfile.txt
123:0:10:1
124:1:20:0

% supervisor < inputfile.txt
0:123:1:false:null:preempted
1:124:19:false:null:running
2:124:18:false:null:running
3:124:17:false:null:running
4:124:16:true:null:wait
5:123:8:false:null:running
6:123:7:false:0:preempted
7:124:15:true:null:wait
...

Assumindo que durante o clock tick 45 o processo 123 está executando, não requisita I/O, tem 15 clock ticks restantes até terminar, não usou seu quantum (dependente de política de escalonamento) e os processos 125, 127 e 200 tem seus pedidos de I/O atendidos, a linha de saída seria:
45:123:15:false:125,127,200:running

Se no ciclo 47 nenhum processo está executando e o processo 148 teve seu I/O terminado a saída seria (pid e tempo restante são null pois não há processo em execução nesse ciclo):
47:null:null:false:148:idle


Opções de linha de comando:
"-F" escalonamento por Filas Múltiplas com Prioridade
"-S [a]" escalonamento por Shortest Job First. Se o valor "a" estiver presente, deve ser usado como fator de aging e estar entre 0.0 e 1.0
"-v" modo verbose

Se nenhuma destas opções for fornecida, o padrão é escalonar por Filas Múltiplas.


Algoritmos de Escalonamento:

Multiplas Filas com prioridade

Processos são escalonados inicialmente de acordo com sua prioridade, que pode variar de 0 (mais alta) a 7 (mais baixa). Quando um processo inicia um burst porque finalizou um I/O, ele tem prioridade 0.
O escalonador deve manter uma fila de processos (FIFO) para nível de prioridade. Ele sempre irá escolher o primeiro processo de mais alto nível disponível para executar a seguir (p.ex. se as filas 0 e 1 estão vazias, mas a 2 não está, o próximo processo será o primeiro da fila 2).
Quando um processo executa, ele irá receber um quantum baseado em sua prioridade. No nível 0 o quantum é de 1, no nível 1 o quantum é 2, no nível 2 o quantum é 4 e assim por diante (se a prioridade for p, o quantum será 2^p).
Se um processo de prioridade p usa todo seu quantum sem bloqueio por I/O ou terminar, o escalonar irá pará-lo, reduzir sua prioridade para p+1 e colocá-lo ao fim da fila daquela prioridade. Se já estiver na fila de menor prioridade, irá para o fim dessa fila.

Preemptive Shortest Job First

Nesta política de escalonamento, a fila de processos Ready consiste em uma fila única, ordenada pelo tempo que o escalonador estima que o processo precisa de CPU. Você deve calcular essa estimativa usando o método da média exponencial (aging). A fórmula neste caso é:
Tn = (a * ta) + ((1 - a) * Ta)
onde "Tn" é a estimativa de tempo para o próximo ciclo; "ta" é o tempo realmente utilizado no último ciclo; "Ta" é a estimativa anterior; "a" é o fator de aging (use 0.5 como default)


Outros requisitos:
Trabalho individual, com sessão de apresentação ao professor a ser marcada posteriormente.
Seu programa deve ser escrito em C e será testado em ambiente Linux com compilador gcc. Use somente as bibliotecas padrão da linguagem.
Para fins de avaliação, seu algoritmo deve chamar a função osPRNG() na ordem descrita no pseudocódigo do Supervisor acima, ou seja, a cada iteração do loop interno você deve chamar IOTerm(), de forma FIFO, para todos os processos na fila de I/O, e depois chamar IOReq() para o processo corrente, nesta ordem.