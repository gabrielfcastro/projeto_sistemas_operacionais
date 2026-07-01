# Pseudo-Sistema Operacional (Pseudo-SO)

# Pseudo-Sistema Operacional (Pseudo-SO)

Este projeto consiste na implementação de um **Pseudo-Sistema Operacional** desenvolvido como trabalho prático para a disciplina de **Sistemas Operacionais** da **Universidade de Brasília (UnB)**. O sistema simula o gerenciamento de processos, memória virtual, alocação de recursos e um sistema de arquivos básico.

O projeto foi desenvolvido pelos alunos:

* **Gabriel Francisco** — Matrícula: **202066571**
* **Daniel Lincoln** — Matrícula: **202042829**
* **Jeferson Augusto** — Matrícula: **160126509**


## 📁 Estrutura do Projeto

* `src/`: Código-fonte principal (`despachante.hpp`, `memoria.hpp`, etc.).
* `tests/`: Testes unitários para cada módulo.
* `test_sincrono/`: Arquivos de configuração para testes síncronos.
* `build/`: Diretório dos binários compilados.

## 🚀 Como Executar

### 1. Compilação

```bash
make
```

### 2. Execução (Cenários de Teste)

Para executar os testes síncronos, coloque arquivos `processes.txt`, `files.txt` e `string.txt` na pasta `test_sincrono/` e execute:

```bash
make run
```

## 🧪 Testes Automatizados

O projeto utiliza uma suíte modular de testes para garantir a integridade de cada componente.

### Executar todos os testes unitários

```bash
make test
```

### Executar um módulo específico

Para validar apenas um componente durante o desenvolvimento ou a avaliação:

```bash
make t_<nome_do_modulo>
```

Exemplos:

```bash
make t_memoria
make t_filas
make t_processo
```

### Executar os testes de integração

```bash
make test_integracao
```

## 🧹 Limpeza

Para remover os binários e arquivos temporários gerados pela compilação:

```bash
make clean
```

## 🛠 Desenvolvimento

Este projeto foi desenvolvido utilizando a abordagem **TDD (Test-Driven Development)**.

Cada módulo possui um arquivo de teste correspondente na pasta `tests/`, permitindo validar sua lógica individualmente antes da integração com os demais componentes.

---

Projeto desenvolvido para a disciplina de **Sistemas Operacionais** da **Universidade de Brasília (UnB)**.
