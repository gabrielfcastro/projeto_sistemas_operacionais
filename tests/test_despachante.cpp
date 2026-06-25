#include "../src/despachante.hpp"
#include "test_framework.hpp"
#include <fstream>
#include <sstream>

using namespace std;

// ============================================================
// Testes do Despachante (despachante.hpp)
// ============================================================

// Helper: escreve um arquivo temporário com o conteúdo dado
static void escrever_arquivo(const string& caminho, const string& conteudo) {
    ofstream f(caminho);
    f << conteudo;
}

// Helper: redireciona stdout para uma string durante o bloco
// (útil para capturar a saída do despachante)
static string capturar_saida(function<void()> fn) {
    streambuf* antigo = cout.rdbuf();
    ostringstream oss;
    cout.rdbuf(oss.rdbuf());
    fn();
    cout.rdbuf(antigo);
    return oss.str();
}

// --- Carregamento (load) ---

void test_carregar_le_dois_processos() {
    escrever_arquivo("/tmp/proc.txt",
        "2, 0, 3, 4, 0, 0, 0, 0\n"
        "8, 0, 2, 8, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",
        "1,2,3,4,1,2,5,1,2,3,4,5\n"
        "7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");

    ASSERT_EQ(2, (int)d.obter_processos().size());
    ASSERT_EQ(0, d.obter_processos()[0].id);
    ASSERT_EQ(1, d.obter_processos()[1].id);
}

void test_carregar_associa_paginas_referenciadas() {
    escrever_arquivo("/tmp/proc.txt", "0, 1, 3, 4, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2,3\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");

    auto& p = d.obter_processos()[0];
    ASSERT_EQ(3, (int)p.lista_de_paginas_referenciadas.size());
    ASSERT_EQ(1, p.lista_de_paginas_referenciadas[0]);
    ASSERT_EQ(2, p.lista_de_paginas_referenciadas[1]);
    ASSERT_EQ(3, p.lista_de_paginas_referenciadas[2]);
}

void test_carregar_arquivo_ausente_lanca_excecao() {
    Despachante d;
    bool lancou = false;
    try { d.carregar_arquivos("/tmp/nao_existe.txt", "/tmp/nao_existe2.txt"); }
    catch (const runtime_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

void test_carregar_falta_linha_paginas_lanca_excecao() {
    escrever_arquivo("/tmp/proc.txt",
        "0, 1, 3, 4, 0, 0, 0, 0\n"
        "0, 1, 2, 4, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt", "1,2,3\n"); // só 1 linha para 2 processos

    Despachante d;
    bool lancou = false;
    try { d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt"); }
    catch (const runtime_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

// --- Saída do despachante ---

void test_formato_cabecalho_despachante() {
    escrever_arquivo("/tmp/proc.txt", "0, 1, 2, 4, 1, 0, 1, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    ASSERT_TRUE(out.find("dispatcher =>")  != string::npos);
    ASSERT_TRUE(out.find("PID: 0")         != string::npos);
    ASSERT_TRUE(out.find("frames: 4")      != string::npos);
    ASSERT_TRUE(out.find("priority: 1")    != string::npos);
    ASSERT_TRUE(out.find("time: 2")        != string::npos);
    ASSERT_TRUE(out.find("printers: 1")    != string::npos);
    ASSERT_TRUE(out.find("scanners: 0")    != string::npos);
    ASSERT_TRUE(out.find("modems: 1")      != string::npos);
    ASSERT_TRUE(out.find("drives: 0")      != string::npos);
}

void test_formato_saida_processo() {
    escrever_arquivo("/tmp/proc.txt", "0, 0, 3, 4, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2,3\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    ASSERT_TRUE(out.find("process 0 =>")     != string::npos);
    ASSERT_TRUE(out.find("P0 STARTED")       != string::npos);
    ASSERT_TRUE(out.find("P0 instruction 1") != string::npos);
    ASSERT_TRUE(out.find("P0 instruction 2") != string::npos);
    ASSERT_TRUE(out.find("P0 instruction 3") != string::npos);
    ASSERT_TRUE(out.find("P0 return SIGINT") != string::npos);
}

void test_saida_exemplo_especificacao() {
    // Exemplo exato da seção 2.1.3 da especificação
    escrever_arquivo("/tmp/proc.txt",
        "2, 0, 3, 4, 0, 0, 0, 0\n"
        "8, 0, 2, 8, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",
        "1,2,3,4,1,2,5,1,2,3,4,5\n"
        "7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // P0 deve aparecer antes de P1 (tempo_chegada 2 < 8)
    ASSERT_TRUE(out.find("P0 STARTED") != string::npos);
    ASSERT_TRUE(out.find("P1 STARTED") != string::npos);
    ASSERT_TRUE(out.find("P0 STARTED") < out.find("P1 STARTED"));

    // P0 tem 3 instruções, P1 tem 2
    ASSERT_TRUE(out.find("P0 instruction 3") != string::npos);
    ASSERT_TRUE(out.find("P1 instruction 2") != string::npos);
    ASSERT_TRUE(out.find("P0 instruction 4") == string::npos);
    ASSERT_TRUE(out.find("P1 instruction 3") == string::npos);
}

// --- Escalonamento ---

void test_tempo_real_executa_antes_de_usuario() {
    // P0 tempo real, P1 usuário, mesmo tempo de chegada (0)
    escrever_arquivo("/tmp/proc.txt",
        "0, 0, 1, 2, 0, 0, 0, 0\n"  // tempo real
        "0, 1, 1, 2, 0, 0, 0, 0\n"); // usuário
    escrever_arquivo("/tmp/str.txt", "1\n1\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    ASSERT_TRUE(out.find("P0 STARTED") < out.find("P1 STARTED"));
}

void test_preempcao_processo_de_usuario() {
    // P0 usuário com 3 ciclos de CPU - deve ser preemptado e voltar à fila
    escrever_arquivo("/tmp/proc.txt", "0, 1, 3, 2, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2,3\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // Todas as 3 instruções devem aparecer (mesmo com preempções)
    ASSERT_TRUE(out.find("P0 instruction 1") != string::npos);
    ASSERT_TRUE(out.find("P0 instruction 2") != string::npos);
    ASSERT_TRUE(out.find("P0 instruction 3") != string::npos);
    ASSERT_TRUE(out.find("P0 return SIGINT") != string::npos);
}

void test_tempo_de_chegada_respeitado() {
    // P0 entra no tempo 0, P1 entra no tempo 5
    escrever_arquivo("/tmp/proc.txt",
        "0, 0, 2, 2, 0, 0, 0, 0\n"
        "5, 0, 1, 2, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt", "1,2\n3\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // P0 deve terminar antes de P1 começar
    size_t p0_fim    = out.find("P0 return SIGINT");
    size_t p1_inicio = out.find("P1 STARTED");
    ASSERT_TRUE(p0_fim < p1_inicio);
}

// ============================================================
// main
// ============================================================
int main() {
    cout << "=== Testes: Despachante ===\n";

    RUN_TEST("carregar_arquivos le dois processos",          test_carregar_le_dois_processos);
    RUN_TEST("carregar_arquivos associa paginas",            test_carregar_associa_paginas_referenciadas);
    RUN_TEST("carregar_arquivos lanca excecao se faltar",    test_carregar_arquivo_ausente_lanca_excecao);
    RUN_TEST("carregar_arquivos lanca excecao falta pagina", test_carregar_falta_linha_paginas_lanca_excecao);
    RUN_TEST("formato cabecalho despachante",                test_formato_cabecalho_despachante);
    RUN_TEST("formato saida processo",                       test_formato_saida_processo);
    RUN_TEST("exemplo da spec secao 2.1.3",                  test_saida_exemplo_especificacao);
    RUN_TEST("tempo real executa antes de usuario",          test_tempo_real_executa_antes_de_usuario);
    RUN_TEST("preempcao processo de usuario",                test_preempcao_processo_de_usuario);
    RUN_TEST("tempo de chegada e respeitado",                test_tempo_de_chegada_respeitado);

    PRINT_RESULTS();
}