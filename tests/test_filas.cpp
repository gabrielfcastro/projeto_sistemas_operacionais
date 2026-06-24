#include "test_framework.hpp"
#include "../src/filas.hpp"
#include <stdexcept>
#include <iostream>

static BCP criar_processo(int id, int prioridade, int tempo_cpu = 3) {
    BCP p;
    p.id                           = id;
    p.prioridade_base              = prioridade;
    p.prioridade_dinamica          = prioridade;
    p.tempo_processador_necessario = tempo_cpu;
    p.tempo_processador_restante   = tempo_cpu;
    p.tempo_chegada                = 0;
    p.working_set                  = 2;
    p.tempo_esperando              = 0;
    p.estado_atual                 = EstadoProcesso::NOVO;
    p.tipo                         = (prioridade == 0) ? TipoProcesso::TEMPO_REAL : TipoProcesso::USUARIO;
    return p;
}

// --- Inserção e Remoção Básico ---

void test_inserir_tempo_real() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 0);
    q.adicionar_processos_na_fila(p);
    ASSERT_EQ(1, q.tamanho_fila_tempo_real());
    ASSERT_EQ(1, q.quantidade_total_processos());
}

void test_inserir_filas_usuario() {
    GerenciadorDeFilas q;
    BCP p1 = criar_processo(0, 1);
    BCP p2 = criar_processo(1, 2);
    BCP p3 = criar_processo(2, 3);
    q.adicionar_processos_na_fila(p1);
    q.adicionar_processos_na_fila(p2);
    q.adicionar_processos_na_fila(p3);
    ASSERT_EQ(1, q.tamanho_fila_usuario(1));
    ASSERT_EQ(1, q.tamanho_fila_usuario(2));
    ASSERT_EQ(1, q.tamanho_fila_usuario(3));
    ASSERT_EQ(3, q.quantidade_total_processos());
}

void test_ordem_fifo_tempo_real() {
    GerenciadorDeFilas q;
    BCP p0 = criar_processo(0, 0);
    BCP p1 = criar_processo(1, 0);
    BCP p2 = criar_processo(2, 0);
    q.adicionar_processos_na_fila(p0);
    q.adicionar_processos_na_fila(p1);
    q.adicionar_processos_na_fila(p2);

    // Deve sair na ordem de entrada (FIFO)
    ASSERT_EQ(0, q.proximo_processo_da_fila_a_ser_executado().id);
    ASSERT_EQ(1, q.proximo_processo_da_fila_a_ser_executado().id);
    ASSERT_EQ(2, q.proximo_processo_da_fila_a_ser_executado().id);
}

void test_tempo_real_tem_prioridade_absoluta() {
    GerenciadorDeFilas q;
    // Enfileira usuário primeiro, depois tempo real
    BCP user = criar_processo(10, 1);
    BCP rt   = criar_processo(20, 0);
    q.adicionar_processos_na_fila(user);
    q.adicionar_processos_na_fila(rt);

    // Tempo real deve sair primeiro independente da ordem de inserção
    BCP first = q.proximo_processo_da_fila_a_ser_executado();
    ASSERT_EQ(20, first.id);
    ASSERT_TRUE(first.tipo == TipoProcesso::TEMPO_REAL);
}

void test_ordem_de_prioridade_usuario() {
    GerenciadorDeFilas q;
    BCP p3 = criar_processo(30, 3);
    BCP p1 = criar_processo(10, 1);
    BCP p2 = criar_processo(20, 2);
    q.adicionar_processos_na_fila(p3);
    q.adicionar_processos_na_fila(p1);
    q.adicionar_processos_na_fila(p2);

    // Deve sair em ordem de prioridade: 1 -> 2 -> 3
    ASSERT_EQ(10, q.proximo_processo_da_fila_a_ser_executado().id);
    ASSERT_EQ(20, q.proximo_processo_da_fila_a_ser_executado().id);
    ASSERT_EQ(30, q.proximo_processo_da_fila_a_ser_executado().id);
}

void test_remocao_seta_estado_executando() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 1);
    q.adicionar_processos_na_fila(p);
    BCP out = q.proximo_processo_da_fila_a_ser_executado();
    ASSERT_TRUE(out.estado_atual == EstadoProcesso::EXECUTANDO);
}

void test_insercao_seta_estado_pronto() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 1);
    q.adicionar_processos_na_fila(p);

    // Inspeciona a fila para ver o estado sem remover
    auto v = q.processos_da_fila_de_usuario(1);
    ASSERT_EQ(1, (int)v.size());
    ASSERT_TRUE(v[0].estado_atual == EstadoProcesso::PRONTO);
}

void test_filas_vazias() {
    GerenciadorDeFilas q;
    ASSERT_TRUE(q.todas_as_filas_estao_vazias());
    ASSERT_TRUE(q.fila_tempo_real_esta_vazia());
    ASSERT_TRUE(q.filas_usuario_estao_vazias());

    BCP p = criar_processo(0, 1);
    q.adicionar_processos_na_fila(p);
    ASSERT_FALSE(q.todas_as_filas_estao_vazias());

    q.proximo_processo_da_fila_a_ser_executado();
    ASSERT_TRUE(q.todas_as_filas_estao_vazias());
}

void test_remocao_em_fila_vazia_lanca_excecao() {
    GerenciadorDeFilas q;
    bool lancou = false;
    try { q.proximo_processo_da_fila_a_ser_executado(); }
    catch (const std::underflow_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

void test_estouro_lanca_excecao() {
    GerenciadorDeFilas q;
    bool lancou = false;
    try {
        for (int i = 0; i <= MAXIMO_DE_PROCESSOS; i++) {
            BCP p = criar_processo(i, 1);
            q.adicionar_processos_na_fila(p);
        }
    } catch (const std::overflow_error&) {
        lancou = true;
    }
    ASSERT_TRUE(lancou);
}

// --- Devolução após preempção ---

void test_devolucao_apos_preempcao() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 1);
    q.adicionar_processos_na_fila(p);
    BCP executando = q.proximo_processo_da_fila_a_ser_executado();
    ASSERT_EQ(0, q.quantidade_total_processos());

    // Simula fim do quantum: reinsere o processo
    q.excedido_o_quantum(executando);
    ASSERT_EQ(1, q.tamanho_fila_usuario(1));
    ASSERT_EQ(0, q.processos_da_fila_de_usuario(1)[0].tempo_esperando); // estresse resetado
}

// --- Envelhecimento ---

void test_envelhecimento_aumenta_tempo_de_espera() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 3); // fila de menor prioridade
    q.adicionar_processos_na_fila(p);

    q.envelhecimento(); // 1 quantum de espera

    auto v = q.processos_da_fila_de_usuario(3);
    ASSERT_EQ(1, v[0].tempo_esperando);
}

void test_envelhecimento_promove_processo_apos_limite() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 3);
    q.adicionar_processos_na_fila(p);

    // Aplica envelhecimento TEMPO_MAXIMO_DE_ESPERA vezes (= 5)
    for (int i = 0; i < TEMPO_MAXIMO_DE_ESPERA; i++)
        q.envelhecimento();

    // Processo deve ter subido para fila 2
    ASSERT_EQ(0, q.tamanho_fila_usuario(3));
    ASSERT_EQ(1, q.tamanho_fila_usuario(2));

    auto v = q.processos_da_fila_de_usuario(2);
    ASSERT_EQ(2, v[0].prioridade_dinamica); // crachá atualizado
    ASSERT_EQ(0, v[0].tempo_esperando);     // contador zerado
}

void test_envelhecimento_promove_duas_vezes() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 3);
    q.adicionar_processos_na_fila(p);

    // Primeira promoção: fila 3 -> 2
    for (int i = 0; i < TEMPO_MAXIMO_DE_ESPERA; i++)
        q.envelhecimento();

    ASSERT_EQ(1, q.tamanho_fila_usuario(2));

    // Segunda promoção: fila 2 -> 1
    for (int i = 0; i < TEMPO_MAXIMO_DE_ESPERA; i++)
        q.envelhecimento();

    ASSERT_EQ(0, q.tamanho_fila_usuario(2));
    ASSERT_EQ(1, q.tamanho_fila_usuario(1));
    ASSERT_EQ(1, q.processos_da_fila_de_usuario(1)[0].prioridade_dinamica);
}

void test_envelhecimento_nao_promove_acima_de_prioridade_1() {
    GerenciadorDeFilas q;
    BCP p = criar_processo(0, 1); // já está na fila mais alta de usuário
    q.adicionar_processos_na_fila(p);

    // Mesmo após muitos quanta, não sai da fila 1
    for (int i = 0; i < TEMPO_MAXIMO_DE_ESPERA * 3; i++)
        q.envelhecimento();

    ASSERT_EQ(1, q.tamanho_fila_usuario(1));
    ASSERT_EQ(1, q.processos_da_fila_de_usuario(1)[0].prioridade_dinamica);
}

void test_envelhecimento_nao_afeta_tempo_real() {
    GerenciadorDeFilas q;
    BCP rt = criar_processo(0, 0);
    q.adicionar_processos_na_fila(rt);

    for (int i = 0; i < TEMPO_MAXIMO_DE_ESPERA * 2; i++)
        q.envelhecimento();

    // Fila de tempo real intacta
    ASSERT_EQ(1, q.tamanho_fila_tempo_real());
}

void test_envelhecimento_multiplos_processos() {
    GerenciadorDeFilas q;
    BCP p1 = criar_processo(0, 3);
    BCP p2 = criar_processo(1, 3);
    q.adicionar_processos_na_fila(p1);
    q.adicionar_processos_na_fila(p2);

    for (int i = 0; i < TEMPO_MAXIMO_DE_ESPERA; i++)
        q.envelhecimento();

    // Ambos devem ter subido para fila 2
    ASSERT_EQ(0, q.tamanho_fila_usuario(3));
    ASSERT_EQ(2, q.tamanho_fila_usuario(2));
}

// ============================================================
// main
// ============================================================
int main() {
    std::cout << "=== Testes: Módulo de Filas ===\n";

    RUN_TEST("inserir tempo real",                        test_inserir_tempo_real);
    RUN_TEST("inserir filas de usuario",                  test_inserir_filas_usuario);
    RUN_TEST("ordem FIFO na fila de tempo real",          test_ordem_fifo_tempo_real);
    RUN_TEST("tempo real tem prioridade absoluta",        test_tempo_real_tem_prioridade_absoluta);
    RUN_TEST("ordem entre filas de usuario",              test_ordem_de_prioridade_usuario);
    RUN_TEST("remocao seta estado EXECUTANDO",            test_remocao_seta_estado_executando);
    RUN_TEST("insercao seta estado PRONTO",               test_insercao_seta_estado_pronto);
    RUN_TEST("todas as filas vazias correto",             test_filas_vazias);
    RUN_TEST("remocao em fila vazia lanca excecao",       test_remocao_em_fila_vazia_lanca_excecao);
    RUN_TEST("estouro lanca excecao",                     test_estouro_lanca_excecao);
    RUN_TEST("devolucao apos preempcao",                  test_devolucao_apos_preempcao);
    RUN_TEST("envelhecimento incrementa espera",          test_envelhecimento_aumenta_tempo_de_espera);
    RUN_TEST("envelhecimento promove apos limite",        test_envelhecimento_promove_processo_apos_limite);
    RUN_TEST("envelhecimento promove duas vezes",         test_envelhecimento_promove_duas_vezes);
    RUN_TEST("envelhecimento nao sobe alem de prio 1",    test_envelhecimento_nao_promove_acima_de_prioridade_1);
    RUN_TEST("envelhecimento nao afeta tempo real",       test_envelhecimento_nao_afeta_tempo_real);
    RUN_TEST("envelhecimento com multiplos processos",    test_envelhecimento_multiplos_processos);

    PRINT_RESULTS();
}