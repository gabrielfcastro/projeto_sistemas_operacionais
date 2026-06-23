#include <iostream>
#include <stdexcept>
#include "test_framework.hpp"
#include "../src/processo.hpp"

using namespace std;

void test_parse_realtime_process() {
    // Processo de tempo real: prioridade 0
    BCP p = Leitor::ler_linha("2, 0, 3, 4, 0, 0, 0, 0", 0);

    ASSERT_EQ(0, p.id);
    ASSERT_EQ(0, p.prioridade_base);
    ASSERT_EQ(0, p.prioridade_dinamica);
    ASSERT_EQ(2, p.tempo_chegada);
    ASSERT_EQ(3, p.tempo_processador_necessario);
    ASSERT_EQ(3, p.tempo_processador_restante);
    ASSERT_EQ(4, p.working_set);
    ASSERT_EQ(0, p.impressora);
    ASSERT_EQ(0, p.scanner);
    ASSERT_EQ(0, p.modem);
    ASSERT_EQ(0, p.sata);
    ASSERT_TRUE(p.tipo  == TipoProcesso::TEMPO_REAL);
    ASSERT_TRUE(p.estado_atual == EstadoProcesso::NOVO);
}

void test_parse_user_process() {
    // Processo de tempo real (Prioridade 0, mas PID 1)
    BCP p = Leitor::ler_linha("8, 0, 2, 8, 0, 0, 0, 0", 1);

    ASSERT_EQ(1, p.id);
    ASSERT_EQ(0, p.prioridade_base);       // prioridade 0 ainda é tempo real
    ASSERT_EQ(8, p.tempo_chegada);
    ASSERT_EQ(2, p.tempo_processador_necessario);
    ASSERT_EQ(8, p.working_set);
    ASSERT_TRUE(p.tipo == TipoProcesso::TEMPO_REAL);
}

void test_parse_user_process_priority1() {
    // Processo de usuário com prioridade 1
    BCP p = Leitor::ler_linha("0, 1, 5, 3, 1, 0, 1, 0", 2);

    ASSERT_EQ(2,  p.id);
    ASSERT_EQ(1,  p.prioridade_base);
    ASSERT_EQ(1,  p.prioridade_dinamica);
    ASSERT_EQ(5,  p.tempo_processador_necessario);
    ASSERT_EQ(1,  p.impressora);
    ASSERT_EQ(0,  p.scanner);
    ASSERT_EQ(1,  p.modem);
    ASSERT_EQ(0,  p.sata);
    ASSERT_TRUE(p.tipo == TipoProcesso::USUARIO);
}

void test_parse_all_resources() {
    // Processo usando todos os recursos de E/S
    BCP p = Leitor::ler_linha("0, 2, 10, 5, 1, 1, 1, 1", 0);

    ASSERT_EQ(1, p.impressora);
    ASSERT_EQ(1, p.scanner);
    ASSERT_EQ(1, p.modem);
    ASSERT_EQ(1, p.sata);
    ASSERT_TRUE(p.tipo == TipoProcesso::USUARIO);
}

void test_parse_spaces_are_trimmed() {
    // Garante que espaços extras ao redor dos valores são ignorados
    BCP p = Leitor::ler_linha("  5 ,  3 ,  7 ,  2 ,  0 ,  0 ,  0 ,  0  ", 0);
    ASSERT_EQ(5, p.tempo_chegada);
    ASSERT_EQ(3, p.prioridade_base);
    ASSERT_EQ(7, p.tempo_processador_necessario);
    ASSERT_EQ(2, p.working_set);
}

void test_parse_invalid_field_count() {
    // Deve lançar exceção se o número de campos estiver errado
    bool threw = false;
    try {
        Leitor::ler_linha("0, 1, 5, 3, 1, 0", 0); // só 6 campos
    } catch (const invalid_argument&) {
        threw = true;
    }
    ASSERT_TRUE(threw);
}

void test_parse_invalid_priority() {
    // Prioridade fora do intervalo [0,3] deve lançar exceção
    bool threw = false;
    try {
        Leitor::ler_linha("0, 5, 5, 3, 0, 0, 0, 0", 0); // prioridade 5
    } catch (const invalid_argument&) {
        threw = true;
    }
    ASSERT_TRUE(threw);
}

// --- carregar_arquivo_processos ---

void test_parse_file_two_processes() {
    // Exemplo direto da especificação (seção 2.1.3)
    string content =
        "2, 0, 3, 4, 0, 0, 0, 0\n"
        "8, 0, 2, 8, 0, 0, 0, 0\n";

    auto procs = Leitor::carregar_arquivo_processos(content);

    ASSERT_EQ(2, (int)procs.size());

    // P0
    ASSERT_EQ(0, procs[0].id);
    ASSERT_EQ(0, procs[0].prioridade_base);
    ASSERT_EQ(3, procs[0].tempo_processador_necessario);
    ASSERT_EQ(4, procs[0].working_set);

    // P1
    ASSERT_EQ(1, procs[1].id);
    ASSERT_EQ(0, procs[1].prioridade_base);
    ASSERT_EQ(2, procs[1].tempo_processador_necessario);
    ASSERT_EQ(8, procs[1].working_set);
}

void test_parse_file_ignores_blank_lines() {
    string content =
        "2, 0, 3, 4, 0, 0, 0, 0\n"
        "\n"
        "8, 0, 2, 8, 0, 0, 0, 0\n"
        "\n";

    auto procs = Leitor::carregar_arquivo_processos(content);
    ASSERT_EQ(2, (int)procs.size());
    ASSERT_EQ(0, procs[0].id);
    ASSERT_EQ(1, procs[1].id);
}

void test_parse_file_pids_are_sequential() {
    string content =
        "0, 1, 2, 2, 0, 0, 0, 0\n"
        "0, 2, 3, 2, 0, 0, 0, 0\n"
        "0, 3, 4, 2, 0, 0, 0, 0\n";

    auto procs = Leitor::carregar_arquivo_processos(content);
    ASSERT_EQ(3, (int)procs.size());
    ASSERT_EQ(0, procs[0].id);
    ASSERT_EQ(1, procs[1].id);
    ASSERT_EQ(2, procs[2].id);
}

void test_pcb_default_state() {
    // Um BCP recém-lido deve estar no estado NOVO com page_faults = 0
    BCP p = Leitor::ler_linha("0, 1, 5, 3, 0, 0, 0, 0", 0);
    ASSERT_TRUE(p.estado_atual == EstadoProcesso::NOVO);
    ASSERT_EQ(0, p.cont_de_pgs_faltantes);
    ASSERT_TRUE(p.tabela_de_paginas.empty());
    ASSERT_TRUE(p.lista_de_paginas_referenciadas.empty());
}

// ============================================================
// main — executa todos os testes
// ============================================================
int main() {
    cout << "=== Testes: Módulo de Processos ===\n";

    RUN_TEST("parse processo de tempo real",        test_parse_realtime_process);
    RUN_TEST("parse processo de tempo real (PID 1)",test_parse_user_process);
    RUN_TEST("parse processo de usuário (prio 1)",  test_parse_user_process_priority1);
    RUN_TEST("parse todos os recursos de E/S",      test_parse_all_resources);
    RUN_TEST("espaços extras são ignorados",        test_parse_spaces_are_trimmed);
    RUN_TEST("exceção — campos insuficientes",      test_parse_invalid_field_count);
    RUN_TEST("exceção — prioridade inválida",       test_parse_invalid_priority);
    RUN_TEST("parse arquivo — dois processos",      test_parse_file_two_processes);
    RUN_TEST("parse arquivo — ignora linhas vazias",test_parse_file_ignores_blank_lines);
    RUN_TEST("parse arquivo — PIDs sequenciais",    test_parse_file_pids_are_sequential);
    RUN_TEST("BCP estado inicial correto",          test_pcb_default_state);

    PRINT_RESULTS();
}