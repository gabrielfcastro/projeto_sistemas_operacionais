#include "test_framework.hpp"
#include "../src/filesystem.hpp"
#include <iostream>

// Monta o cenário inicial do exemplo da especificação: disco de 10 blocos com
// os arquivos X (blocos 0-1), Y (bloco 3) e Z (blocos 5-7).
static SistemaDeArquivos cenario_exemplo() {
    SistemaDeArquivos sistema(10);
    sistema.adicionar_arquivo_inicial('X', 0, 2);
    sistema.adicionar_arquivo_inicial('Y', 3, 1);
    sistema.adicionar_arquivo_inicial('Z', 5, 3);
    return sistema;
}

// first-fit deve falhar quando não há sequência contígua grande o bastante.
static void test_criar_sem_espaco() {
    SistemaDeArquivos sistema = cenario_exemplo();
    InfoProcessos info{ { true, true } }; // P0 e P1, ambos tempo real.

    // Não existem 5 blocos contíguos livres -> falta de espaço.
    ResultadoOperacao r = sistema.criar(0, info, 'A', 5);
    ASSERT_FALSE(r.sucesso);
    ASSERT_EQ(string("O processo 0 não pode criar o arquivo A (falta de espaço)."),
              r.mensagem);
}

// Processo de tempo real pode deletar arquivo que não criou (ex.: X, dono -1).
static void test_tempo_real_deleta_qualquer_arquivo() {
    SistemaDeArquivos sistema = cenario_exemplo();
    InfoProcessos info{ { true, true } };

    ResultadoOperacao r = sistema.deletar(0, info, 'X');
    ASSERT_TRUE(r.sucesso);
    ASSERT_EQ(string("O processo 0 deletou o arquivo X."), r.mensagem);
    // Inicialmente 4 blocos livres (X,Y,Z ocupam 6 de 10); deletar X libera +2.
    ASSERT_EQ(6, sistema.blocos_livres());
}

// Operação referenciando um PID inexistente deve falhar com a mensagem padrão.
static void test_processo_inexistente() {
    SistemaDeArquivos sistema = cenario_exemplo();
    InfoProcessos info{ { true, true } }; // Só existem P0 e P1.

    ResultadoOperacao r = sistema.criar(2, info, 'B', 2);
    ASSERT_FALSE(r.sucesso);
    ASSERT_EQ(string("O processo 2 não existe."), r.mensagem);
}

// first-fit aloca a partir do primeiro bloco livre suficiente.
static void test_criar_first_fit() {
    SistemaDeArquivos sistema = cenario_exemplo();
    InfoProcessos info{ { true, true } };

    // Libera X (blocos 0-1); agora 0,1,2 ficam contíguos e livres.
    sistema.deletar(0, info, 'X');

    ResultadoOperacao r = sistema.criar(0, info, 'D', 3);
    ASSERT_TRUE(r.sucesso);
    ASSERT_EQ(string("O processo 0 criou o arquivo D (blocos 0, 1 e 2)."),
              r.mensagem);
}

// Usuário comum não pode deletar arquivo que não criou.
static void test_usuario_nao_deleta_de_outro() {
    SistemaDeArquivos sistema(10);
    // P0 usuário, P1 usuário.
    InfoProcessos info{ { false, false } };

    // P0 cria o arquivo F.
    ASSERT_TRUE(sistema.criar(0, info, 'F', 2).sucesso);

    // P1 (usuário) tenta deletar F, criado por P0 -> negado.
    ResultadoOperacao negado = sistema.deletar(1, info, 'F');
    ASSERT_FALSE(negado.sucesso);

    // P0, dono do arquivo, consegue deletar.
    ASSERT_TRUE(sistema.deletar(0, info, 'F').sucesso);
}

// Deletar arquivo inexistente falha com a mensagem apropriada.
static void test_deletar_inexistente() {
    SistemaDeArquivos sistema = cenario_exemplo();
    InfoProcessos info{ { true, true } };

    ResultadoOperacao r = sistema.deletar(1, info, 'E');
    ASSERT_FALSE(r.sucesso);
    ASSERT_EQ(string("O processo 1 não pode deletar o arquivo E porque ele não existe."),
              r.mensagem);
}

// O mapa do disco mostra a letra dos arquivos e 0 para blocos vazios.
static void test_mapa_do_disco() {
    SistemaDeArquivos sistema = cenario_exemplo();
    InfoProcessos info{ { true, true } };

    sistema.deletar(0, info, 'X');   // Libera 0-1.
    sistema.criar(0, info, 'D', 3);  // Ocupa 0-2 com D.

    // Esperado: D D D Y 0 Z Z Z 0 0
    ASSERT_EQ(string("| D | D | D | Y | 0 | Z | Z | Z | 0 | 0 |"),
              sistema.mapa_do_disco());
}

// Executa o pipeline completo a partir do conteúdo de um files.txt.
static void test_processar_arquivo_completo() {
    string conteudo =
        "10\n"
        "3\n"
        "X, 0, 2\n"
        "Y, 3, 1\n"
        "Z, 5, 3\n"
        "0, 0, A, 5\n"   // P0 cria A (5 blocos) -> falta de espaço
        "0, 1, X\n"      // P0 deleta X -> sucesso
        "2, 0, B, 2\n"   // P2 cria B -> processo não existe
        "0, 0, D, 3\n"   // P0 cria D (3 blocos) -> sucesso (blocos 0,1,2)
        "1, 1, E\n";     // P1 deleta E -> não existe

    InfoProcessos info{ { true, true } }; // P0 e P1 (ambos tempo real no exemplo).
    string saida = LeitorArquivos::processar(conteudo, info);

    // Confere alguns trechos-chave da saída formatada.
    ASSERT_TRUE(saida.find("Sistema de arquivos =>") != string::npos);
    ASSERT_TRUE(saida.find("O processo 0 não pode criar o arquivo A (falta de espaço).")
                != string::npos);
    ASSERT_TRUE(saida.find("O processo 0 deletou o arquivo X.") != string::npos);
    ASSERT_TRUE(saida.find("O processo 2 não existe.") != string::npos);
    ASSERT_TRUE(saida.find("O processo 0 criou o arquivo D (blocos 0, 1 e 2).")
                != string::npos);
    ASSERT_TRUE(saida.find("| D | D | D | Y | 0 | Z | Z | Z | 0 | 0 |")
                != string::npos);
}

int main() {
    RUN_TEST("Criar sem espaço (first-fit)",          test_criar_sem_espaco);
    RUN_TEST("Tempo real deleta qualquer arquivo",    test_tempo_real_deleta_qualquer_arquivo);
    RUN_TEST("Processo inexistente",                  test_processo_inexistente);
    RUN_TEST("Criar com first-fit",                   test_criar_first_fit);
    RUN_TEST("Usuário não deleta arquivo de outro",   test_usuario_nao_deleta_de_outro);
    RUN_TEST("Deletar arquivo inexistente",           test_deletar_inexistente);
    RUN_TEST("Mapa de ocupação do disco",             test_mapa_do_disco);
    RUN_TEST("Processar files.txt completo",          test_processar_arquivo_completo);
    PRINT_RESULTS();
}
