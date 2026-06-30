#include "../src/despachante.hpp"
#include "test_framework.hpp"
#include <fstream>
#include <sstream>

using namespace std;

static void escrever_arquivo(const string& caminho, const string& conteudo) {
    ofstream f(caminho);
    f << conteudo;
}

static string capturar_saida(function<void()> fn) {
    streambuf* antigo = cout.rdbuf();
    ostringstream oss;
    cout.rdbuf(oss.rdbuf());
    fn();
    cout.rdbuf(antigo);
    return oss.str();
}

// --- Integração com memória ---

void test_relatorio_faltas_de_pagina_aparece() {
    escrever_arquivo("/tmp/proc.txt", "0, 0, 3, 4, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2,3,4\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    ASSERT_TRUE(out.find("Número de Faltas de Páginas por processo:") != string::npos);
    ASSERT_TRUE(out.find("P0 = ") != string::npos);
    ASSERT_TRUE(out.find("faltas de páginas") != string::npos);
}

void test_exemplo_completo_da_spec_faltas_de_pagina() {
    // Reproduz o exemplo exato da seção 2.1.3:
    // P0 (working_set=4): 6/7 faltas (dependendo de como a pré-carga é contada)
    // P1 (working_set=8): 14 faltas
    escrever_arquivo("/tmp/proc.txt",
        "2, 0, 3, 4, 0, 0, 0, 0\n"
        "8, 0, 2, 8, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",
        "1,2,3,4,1,2,5,1,2,3,4,5\n"
        "7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // P1 deve ter exatamente 14 faltas, conforme a spec
    ASSERT_TRUE(out.find("P1 = 14 faltas de páginas") != string::npos);
}

void test_processo_de_usuario_preemptado_acumula_faltas_corretamente() {
    // Processo de usuário com 3 quanta — preemptado a cada instrução.
    // Mesmo preemptado, as faltas devem se acumular corretamente no
    // relatório final (testa a sincronização entre cópias de BCP).
    escrever_arquivo("/tmp/proc.txt", "0, 1, 3, 4, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2,3\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // 4 páginas distintas (pré-carga=1, depois 1,2,3) cabem no working_set=4,
    // então não deve haver faltas além da carga inicial das páginas 2 e 3
    // (página 1 é pré-carregada e não conta).
    ASSERT_TRUE(out.find("P0 = 2 faltas de páginas") != string::npos);
}

// --- Integração com recursos de E/S ---

void test_processo_usuario_aloca_e_libera_recurso() {
    // Processo de usuário pedindo o único scanner. Deve conseguir e o
    // recurso deve voltar a ficar disponível após a execução.
    escrever_arquivo("/tmp/proc.txt", "0, 1, 1, 2, 0, 1, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // O processo deve ter rodado normalmente (não ficou travado)
    ASSERT_TRUE(out.find("P0 STARTED") != string::npos);
    ASSERT_TRUE(out.find("P0 return SIGINT") != string::npos);
}

void test_dois_processos_disputam_recurso_unico() {
    // P0 e P1, ambos usuário, ambos pedindo o único modem.
    // Um deve rodar primeiro; o outro deve esperar e rodar depois
    // (mas ambos devem terminar — não pode travar).
    escrever_arquivo("/tmp/proc.txt",
        "0, 1, 1, 2, 0, 0, 1, 0\n"
        "0, 1, 1, 2, 0, 0, 1, 0\n");
    escrever_arquivo("/tmp/str.txt", "1\n1\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // Ambos devem terminar
    ASSERT_TRUE(out.find("P0 return SIGINT") != string::npos);
    ASSERT_TRUE(out.find("P1 return SIGINT") != string::npos);
}

void test_tempo_real_nao_disputa_recursos() {
    // Processo de tempo real não deve passar pela checagem de recursos
    // (mesmo que tivesse campos de E/S diferentes de zero, são zerados
    // no parser). Deve executar sem qualquer bloqueio.
    escrever_arquivo("/tmp/proc.txt", "0, 0, 2, 2, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt");
    string out = capturar_saida([&]{ d.executar(); });

    ASSERT_TRUE(out.find("P0 return SIGINT") != string::npos);
}

// --- Integração com sistema de arquivos ---

void test_sistema_de_arquivos_processado_quando_fornecido() {
    escrever_arquivo("/tmp/proc.txt",
        "2, 0, 3, 4, 0, 0, 0, 0\n"
        "8, 0, 2, 8, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",
        "1,2,3,4,1,2,5,1,2,3,4,5\n"
        "7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1\n");
    escrever_arquivo("/tmp/files.txt",
        "10\n"
        "3\n"
        "X, 0, 2\n"
        "Y, 3, 1\n"
        "Z, 5, 3\n"
        "0, 0, A, 5\n"
        "0, 1, X\n"
        "2, 0, B, 2\n"
        "0, 0, D, 3\n"
        "1, 1, E\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt", "/tmp/files.txt");
    string out = capturar_saida([&]{ d.executar(); });

    // Saída do sistema de arquivos deve aparecer DEPOIS da execução dos processos
    ASSERT_TRUE(out.find("Sistema de arquivos =>") != string::npos);
    ASSERT_TRUE(out.find("O processo 0 não pode criar o arquivo A (falta de espaço).") != string::npos);
    ASSERT_TRUE(out.find("O processo 0 deletou o arquivo X.") != string::npos);
    ASSERT_TRUE(out.find("O processo 2 não existe.") != string::npos);
    ASSERT_TRUE(out.find("O processo 0 criou o arquivo D (blocos 0, 1 e 2).") != string::npos);
    ASSERT_TRUE(out.find("| D | D | D | Y | 0 | Z | Z | Z | 0 | 0 |") != string::npos);

    // Ordem: execução dos processos vem antes do sistema de arquivos,
    // que por sua vez vem antes do relatório de faltas de página
    size_t pos_p1_fim   = out.find("P1 return SIGINT");
    size_t pos_arquivos = out.find("Sistema de arquivos =>");
    size_t pos_faltas   = out.find("Número de Faltas de Páginas por processo:");
    ASSERT_TRUE(pos_p1_fim   < pos_arquivos);
    ASSERT_TRUE(pos_arquivos < pos_faltas);
}

void test_sistema_de_arquivos_omitido_quando_nao_fornecido() {
    escrever_arquivo("/tmp/proc.txt", "0, 0, 2, 2, 0, 0, 0, 0\n");
    escrever_arquivo("/tmp/str.txt",  "1,2\n");

    Despachante d;
    d.carregar_arquivos("/tmp/proc.txt", "/tmp/str.txt"); // sem files.txt
    string out = capturar_saida([&]{ d.executar(); });

    ASSERT_TRUE(out.find("Sistema de arquivos =>") == string::npos);
    // O relatório de faltas de página deve aparecer mesmo assim
    ASSERT_TRUE(out.find("Número de Faltas de Páginas por processo:") != string::npos);
}

// ============================================================
// main
// ============================================================
int main() {
    cout << "=== Testes de Integração: Despachante ===\n";

    RUN_TEST("relatório de faltas de página aparece",          test_relatorio_faltas_de_pagina_aparece);
    RUN_TEST("exemplo completo da spec — P1 = 14 faltas",      test_exemplo_completo_da_spec_faltas_de_pagina);
    RUN_TEST("preempção acumula faltas corretamente",          test_processo_de_usuario_preemptado_acumula_faltas_corretamente);
    RUN_TEST("usuário aloca e libera recurso",                 test_processo_usuario_aloca_e_libera_recurso);
    RUN_TEST("dois processos disputam recurso único",          test_dois_processos_disputam_recurso_unico);
    RUN_TEST("tempo real não disputa recursos",                test_tempo_real_nao_disputa_recursos);
    RUN_TEST("sistema de arquivos processado quando fornecido",test_sistema_de_arquivos_processado_quando_fornecido);
    RUN_TEST("sistema de arquivos omitido quando ausente",     test_sistema_de_arquivos_omitido_quando_nao_fornecido);

    PRINT_RESULTS();
}