
-- Implementar IO Req (requisição para IO ou não)
-- Implementar IO Term (requisição para IO foi terminada ou não)
-- Implementar -V

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