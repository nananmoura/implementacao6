# Algoritmo de Banker

Implementação 5 de Infraestrutura de Software (CESAR School), em C.

## Compilar

    make

Gera o executável `banker`. Para limpar:

    make clean

## Executar

Cada argumento é o número de instâncias de um tipo de recurso:

    ./banker 10 5 7

O programa lê `customer.txt` e `commands.txt` do diretório atual e grava `result.txt`.

## Arquivos de entrada

`customer.txt`: uma linha por cliente, com o máximo de cada recurso separado por vírgula.

    7,5,3
    3,2,2

`commands.txt`: um comando por linha.

    RQ 0 0 2 1    pede recursos (cliente, quantidades)
    RL 0 0 1 1    libera recursos
    *             imprime o estado (MAXIMUM, ALLOCATION, NEED, AVAILABLE)

## Erros

O programa confere tudo antes de executar. Se faltar arquivo, a formatação estiver errada ou a quantidade de colunas/valores não bater com a quantidade de argumentos, imprime uma mensagem em stderr e encerra com código 1, sem gerar `result.txt`.

## Mensagens do result.txt

- `Allocate to customer ...`: pedido atendido
- `Release from customer ...`: liberação feita
- `... are not enough to customer ...`: não há recursos disponíveis
- `... exceed its maximum allocation`: pedido passa do máximo declarado
- `... result in an unsafe state`: o pedido deixaria o sistema em estado inseguro