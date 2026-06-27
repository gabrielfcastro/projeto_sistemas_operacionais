# Roteiro — Pessoa C (Recursos de E/S + Sistema de Arquivos)

> Documento para alinhar o grupo. A ideia é "conversar com o PDF da
> implementação": para cada parte da minha responsabilidade, mostrar **o que
> entra**, **o que acontece** e **o que sai**. Assim a Pessoa A (dispatcher) e a
> Pessoa B (memória) sabem exatamente como encaixar a minha parte.

---

## 1. Onde a Pessoa C se encaixa no pseudo-SO

O trabalho é um pseudo-SO multiprogramado com 4 gerenciadores. A divisão do grupo:

| Pessoa | Responsabilidade                                  | Arquivos principais |
|--------|---------------------------------------------------|---------------------|
| A      | Processos + Escalonador (núcleo / dispatcher)     | `processo.hpp`, `filas.hpp`, `dispatcher` |
| B      | Memória (paginação + LRU + page faults)           | `memory.hpp` |
| **C**  | **Recursos de E/S + Sistema de Arquivos**         | **`resource.hpp`, `filesystem.hpp`** |

A minha parte é **quase independente**: ela só precisa saber, de cada processo,
**duas coisas que vêm do PCB (BCP)**:
1. Se o processo é de **tempo real** ou de **usuário** (campo `tipo`).
2. Quais recursos de E/S ele **pediu** (campos `impressora`, `scanner`, `modem`, `sata`).

---

## 2. Os 3 arquivos de entrada (Seção 2.1.2 do PDF)

O programa recebe **3 arquivos `.txt`** na linha de comando:

```
$ ./dispatcher processes.txt files.txt string.txt
```

| Arquivo        | Quem usa         | Conteúdo |
|----------------|------------------|----------|
| `processes.txt`| Pessoa A         | Um processo por linha |
| `files.txt`    | **Pessoa C**     | Configuração do disco + operações de arquivo |
| `string.txt`   | Pessoa B         | String de referência de páginas por processo |

A Pessoa C **lê e processa o `files.txt`**. Os outros dois arquivos não me
interessam diretamente — eu só recebo da Pessoa A a informação de tipo/recursos
de cada processo já carregado no PCB.

---

## 3. ENTRADAS e SAÍDAS — Recursos de E/S (`resource.hpp`)

### 3.1 O que existe no sistema (Seção 1.3)

São recursos com quantidade **fixa**, de uso **exclusivo** (um processo por vez)
e **sem preempção** (uma vez alocado, ninguém tira até o processo liberar):

| Recurso     | Unidades |
|-------------|----------|
| scanner     | 1        |
| impressoras | 2        |
| modem       | 1        |
| SATA        | 2        |

> **Regra de ouro:** processos de **tempo real NÃO usam recursos de E/S**. Isso
> já é garantido lá na leitura do processo (Pessoa A zera os pedidos de E/S de
> processos de tempo real em `processo.hpp`).

### 3.2 ENTRADA

Um `BCP` (vindo da Pessoa A) com:
- `tipo` = `TEMPO_REAL` ou `USUARIO`
- `impressora`, `scanner`, `modem`, `sata` = `0` (não quer) ou `1` (quer)

### 3.3 O QUE ACONTECE

Antes de o dispatcher rodar um processo de **usuário**, ele me chama:

```cpp
ResultadoAlocacao r = gerenciador.tentar_alocar(processo);
```

Estratégia: **tudo-ou-nada e não bloqueante**. Eu tento pegar de uma vez TODOS
os recursos que o processo pediu:
- Se conseguir todos → `sucesso = true`, o processo pode executar.
- Se faltar **qualquer um** → devolvo os que já tinha pego (rollback) e retorno
  `sucesso = false`. O dispatcher então **recoloca o processo na fila** (sem
  preempção, ele tenta de novo depois).

Esse "tudo-ou-nada" **evita deadlock**: um processo nunca fica segurando um
recurso enquanto espera por outro.

Quando o processo termina (ou cede a CPU), o dispatcher me chama:

```cpp
gerenciador.liberar(processo);
```

E eu devolvo ao sistema exatamente os recursos que aquele processo havia pedido.

### 3.4 SAÍDA

- `tentar_alocar` → `ResultadoAlocacao { bool sucesso; string mensagem; }`
- Consultas auxiliares: `scanners_disponiveis()`, `impressoras_disponiveis()`,
  `modems_disponiveis()`, `sata_disponiveis()`.

### 3.5 Exemplo prático

```
Estado inicial: scanner=1, impressoras=2, modem=1, sata=2

P1 (usuário) pede scanner+impressora  -> tenta_alocar -> SUCESSO
   restante: scanner=0, impressoras=1, modem=1, sata=2

P2 (usuário) pede scanner             -> tenta_alocar -> FALHA (scanner ocupado)
   (P2 volta para a fila)

P1 termina -> liberar(P1)
   restante: scanner=1, impressoras=2, modem=1, sata=2

P2 tenta de novo -> SUCESSO
```

---

## 4. ENTRADAS e SAÍDAS — Sistema de Arquivos (`filesystem.hpp`)

### 4.1 O modelo (Seção 1.4)

- O disco é uma fileira de **blocos**.
- Cada arquivo ocupa blocos **contíguos** (alocação contígua).
- Para escolher onde gravar, uso **first-fit**: procuro a **primeira** sequência
  livre grande o suficiente, sempre **a partir do bloco 0**.
- O arquivo fica no disco **mesmo depois que o processo termina**.

**Regras de permissão:**

| Operação | Processo de TEMPO REAL        | Processo de USUÁRIO                          |
|----------|-------------------------------|----------------------------------------------|
| Criar    | Pode (se houver espaço)       | Pode, quantos quiser (se houver espaço)      |
| Deletar  | Pode deletar **qualquer** um  | Só pode deletar arquivos **criados por ele** |

### 4.2 ENTRADA: o arquivo `files.txt`

Formato (a Pessoa C interpreta isso):

```
Linha 1        -> Quantidade de blocos do disco
Linha 2        -> Quantidade de arquivos já gravados (n)
Linhas 3..n+2  -> <arquivo>, <primeiro bloco>, <qtd de blocos>   (já no disco)
Linhas n+3..   -> <ID_Processo>, <Código>, <Nome_arquivo>[, <qtd blocos>]
                  Código 0 = CRIAR  (precisa da qtd de blocos)
                  Código 1 = DELETAR (só o nome)
```

Exemplo do PDF (`files.txt`):

```
10            <- disco com 10 blocos
3             <- 3 arquivos já gravados
X, 0, 2       <- X ocupa blocos 0 e 1
Y, 3, 1       <- Y ocupa o bloco 3
Z, 5, 3       <- Z ocupa blocos 5, 6 e 7
0, 0, A, 5    <- operação 1: processo 0 CRIA o arquivo A com 5 blocos
0, 1, X       <- operação 2: processo 0 DELETA o arquivo X
2, 0, B, 2    <- operação 3: processo 2 CRIA o arquivo B com 2 blocos
0, 0, D, 3    <- operação 4: processo 0 CRIA o arquivo D com 3 blocos
1, 1, E       <- operação 5: processo 1 DELETA o arquivo E
```

> Os IDs de processo começam em **0** e vão até **(quantidade de processos − 1)**.
> No exemplo, só existem os processos 0 e 1 (porque `processes.txt` tem 2 linhas).

### 4.3 O QUE ACONTECE (passo a passo do exemplo)

Disco inicial (10 blocos): `X X _ Y _ Z Z Z _ _`

1. **`0, 0, A, 5`** → P0 cria A (5 blocos). Não existem 5 blocos contíguos
   livres → **FALHA (falta de espaço)**.
2. **`0, 1, X`** → P0 deleta X. P0 é tempo real → pode deletar qualquer arquivo →
   **SUCESSO**. Blocos 0 e 1 ficam livres: `_ _ _ Y _ Z Z Z _ _`
3. **`2, 0, B, 2`** → operação do processo 2, que **não existe** (só há P0 e P1) →
   **FALHA (processo não existe)**.
4. **`0, 0, D, 3`** → P0 cria D (3 blocos). First-fit acha os blocos 0,1,2 livres →
   **SUCESSO**: `D D D Y _ Z Z Z _ _`
5. **`1, 1, E`** → P1 deleta E, que **não existe** → **FALHA (arquivo não existe)**.

### 4.4 SAÍDA

A Pessoa C devolve um texto pronto para o dispatcher imprimir:

```
Sistema de arquivos =>

Operação 1 => Falha
O processo 0 não pode criar o arquivo A (falta de espaço).

Operação 2 => Sucesso
O processo 0 deletou o arquivo X.

Operação 3 => Falha
O processo 2 não existe.

Operação 4 => Sucesso
O processo 0 criou o arquivo D (blocos 0, 1 e 2).

Operação 5 => Falha
O processo 1 não pode deletar o arquivo E porque ele não existe.

Mapa de ocupação do disco:
| D | D | D | Y | 0 | Z | Z | Z | 0 | 0 |
```

No mapa, cada célula mostra a **letra do arquivo** ou **`0`** (bloco vazio),
conforme a especificação ("blocos vazios identificados por 0").

---

## 5. Como integrar com a Pessoa A (dispatcher)

A Pessoa C entrega **funções/classes**; quem chama é o dispatcher. O contrato:

### 5.1 Recursos de E/S
```cpp
#include "resource.hpp"

GerenciadorDeRecursos recursos;

// Antes de executar um processo de usuário:
if (recursos.tentar_alocar(proc).sucesso) {
    // ... executa o processo ...
    recursos.liberar(proc);   // ao terminar / ceder CPU
} else {
    // recoloca proc na fila (sem preempção)
}
```

### 5.2 Sistema de Arquivos
```cpp
#include "filesystem.hpp"

// 'info' diz quais PIDs existem e quais são de tempo real.
// O índice é o PID; o tamanho do vetor = quantidade de processos.
InfoProcessos info;
for (const BCP& p : todos_os_processos)
    info.eh_tempo_real.push_back(p.tipo == TipoProcesso::TEMPO_REAL);

// Depois de executar todos os processos, processa o files.txt e imprime:
string conteudo_files = ler_arquivo("files.txt");
cout << LeitorArquivos::processar(conteudo_files, info);
```

> **Importante:** a Pessoa A só precisa me passar `InfoProcessos` (montado a
> partir do `tipo` de cada PCB). O resto do `files.txt` eu mesma leio e
> interpreto. Não há acoplamento com a memória (Pessoa B).

---

## 6. Como testar a minha parte

```bash
make test_resource    # testes do gerenciador de E/S  (27 asserts)
make test_arquivos    # testes do sistema de arquivos (21 asserts, reproduz o exemplo do PDF)
make test             # roda TODOS os módulos do grupo
```

Os testes em `tests/test_files.cpp` reproduzem **exatamente** o exemplo do PDF,
servindo de "prova viva" de que as entradas geram as saídas esperadas.

---

## 7. Resumo de uma frase para cada colega

- **Pessoa A:** "Me chama `tentar_alocar`/`liberar` por processo de usuário, e no
  fim me passa um `InfoProcessos` para eu imprimir o bloco do sistema de arquivos."
- **Pessoa B:** "Não dependo de você; trabalhamos em paralelo. Só compartilhamos o
  mesmo PCB definido pela Pessoa A."
