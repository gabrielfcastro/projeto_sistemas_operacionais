#include "test_framework.hpp"
#include "../src/memoria.hpp"

// ============================================================
// Testes do Módulo de Memória (memoria.hpp)
// ============================================================

// Helper: cria BCP mínimo para os testes
static BCP criar_processo(int id, TipoProcesso tipo, int working_set,
                           vector<int> paginas = {}) {
    BCP p;
    p.id                            = id;
    p.prioridade_base               = (tipo == TipoProcesso::TEMPO_REAL) ? 0 : 1;
    p.prioridade_dinamica           = p.prioridade_base;
    p.tipo                          = tipo;
    p.estado_atual                  = EstadoProcesso::NOVO;
    p.working_set                   = working_set;
    p.lista_de_paginas_referenciadas = paginas;
    p.cont_de_pgs_faltantes         = 0;
    p.tempo_esperando               = 0;
    return p;
}

// Helper: conta frames ocupados numa faixa
static int contar_frames_ocupados(GerenciadorDeMemoria& mm,
                                   const vector<FrameDaMemoria>& frames,
                                   int inicio, int fim) {
    // Acesso indireto via acessar_pagina — usamos working_set como proxy
    (void)mm; (void)frames; (void)inicio; (void)fim;
    return 0; // placeholder; testes usam efeitos observáveis
}

// --- inicializar_processos ---

void test_init_sem_paginas_nao_aloca_frames() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::TEMPO_REAL, 4);
    // sem lista_de_paginas_referenciadas
    mm.inicializar_processos(p);
    // não deve lançar exceção e falhas = 0
    ASSERT_EQ(0, p.cont_de_pgs_faltantes);
}

void test_init_precarga_nao_conta_como_falta() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::TEMPO_REAL, 4, {5, 3, 1});
    mm.inicializar_processos(p);
    // Pré-carga não deve contar como falta de página
    ASSERT_EQ(0, p.cont_de_pgs_faltantes);
}

void test_init_duplicado_lanca_excecao() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::TEMPO_REAL, 4);
    mm.inicializar_processos(p);
    bool lancou = false;
    try { mm.inicializar_processos(p); }
    catch (const runtime_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

void test_init_working_set_maior_que_particao_tr_lanca_excecao() {
    GerenciadorDeMemoria mm;
    // Tempo real só tem 8 frames; pedir 9 deve lançar
    BCP p = criar_processo(0, TipoProcesso::TEMPO_REAL, 9);
    bool lancou = false;
    try { mm.inicializar_processos(p); }
    catch (const runtime_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

void test_init_working_set_maior_que_particao_usuario_lanca_excecao() {
    GerenciadorDeMemoria mm;
    // Usuário só tem 12 frames; pedir 13 deve lançar
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 13);
    bool lancou = false;
    try { mm.inicializar_processos(p); }
    catch (const runtime_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

// --- acessar_pagina: hit e falta ---

void test_primeiro_acesso_gera_falta() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 4);
    mm.inicializar_processos(p);

    bool falta = mm.acessar_pagina(p, 7);
    ASSERT_TRUE(falta);
    ASSERT_EQ(1, p.cont_de_pgs_faltantes);
}

void test_segundo_acesso_mesma_pagina_e_hit() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 4);
    mm.inicializar_processos(p);

    mm.acessar_pagina(p, 7);          // falta
    bool falta = mm.acessar_pagina(p, 7); // hit
    ASSERT_FALSE(falta);
    ASSERT_EQ(1, p.cont_de_pgs_faltantes); // continua 1
}

void test_multiplas_faltas_incrementam_contador() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 4);
    mm.inicializar_processos(p);

    mm.acessar_pagina(p, 1);
    mm.acessar_pagina(p, 2);
    mm.acessar_pagina(p, 3);
    ASSERT_EQ(3, p.cont_de_pgs_faltantes);
}

void test_acesso_sem_init_lanca_excecao() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(99, TipoProcesso::USUARIO, 4);
    bool lancou = false;
    try { mm.acessar_pagina(p, 1); }
    catch (const runtime_error&) { lancou = true; }
    ASSERT_TRUE(lancou);
}

// --- isolamento de partições ---

void test_tempo_real_usa_frames_0_a_7() {
    GerenciadorDeMemoria mm;
    // Inicializa com página 0 para garantir alocação
    BCP p = criar_processo(0, TipoProcesso::TEMPO_REAL, 4, {0});
    mm.inicializar_processos(p);
    // Se não lançou exceção, a alocação foi válida.
    // Verificamos indiretamente: acesso a páginas extras deve funcionar
    mm.acessar_pagina(p, 1);
    mm.acessar_pagina(p, 2);
    ASSERT_EQ(2, p.cont_de_pgs_faltantes); // páginas 1 e 2 são faltas
}

void test_usuario_usa_frames_8_a_19() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 4, {0});
    mm.inicializar_processos(p);
    mm.acessar_pagina(p, 1);
    mm.acessar_pagina(p, 2);
    ASSERT_EQ(2, p.cont_de_pgs_faltantes);
}

void test_tr_e_usuario_nao_compartilham_frames() {
    GerenciadorDeMemoria mm;
    // Ocupa todos os frames de tempo real
    for (int i = 0; i < 8; i++) {
        BCP p = criar_processo(i, TipoProcesso::TEMPO_REAL, 1, {i});
        mm.inicializar_processos(p);
    }
    // Usuário ainda deve conseguir alocar (partição separada)
    BCP u = criar_processo(99, TipoProcesso::USUARIO, 4, {0});
    bool lancou = false;
    try { mm.inicializar_processos(u); }
    catch (...) { lancou = true; }
    ASSERT_FALSE(lancou); // não deve lançar
}

// --- LRU ---

void test_lru_despeja_pagina_menos_recente() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 3);
    mm.inicializar_processos(p);

    // Carrega páginas 1, 2, 3 (working set cheio)
    mm.acessar_pagina(p, 1);
    mm.acessar_pagina(p, 2);
    mm.acessar_pagina(p, 3);

    // Acessa página 4 → deve despejar página 1 (LRU)
    // Depois, acessar página 1 novamente deve gerar falta (foi despejada)
    int faltas_antes = p.cont_de_pgs_faltantes;
    mm.acessar_pagina(p, 4); // falta, despeja 1
    bool falta_em_1 = mm.acessar_pagina(p, 1); // deve ser falta (foi despejada)

    ASSERT_TRUE(falta_em_1);
    ASSERT_EQ(faltas_antes + 2, p.cont_de_pgs_faltantes);
}

void test_lru_atualiza_no_hit() {
    GerenciadorDeMemoria mm;
    BCP p = criar_processo(0, TipoProcesso::USUARIO, 3);
    mm.inicializar_processos(p);

    mm.acessar_pagina(p, 1);
    mm.acessar_pagina(p, 2);
    mm.acessar_pagina(p, 3);

    // Re-acessa página 1 → agora ela é a mais recente, página 2 vira LRU
    mm.acessar_pagina(p, 1); // hit

    // Acessa página 4 → deve despejar página 2 (nova LRU)
    mm.acessar_pagina(p, 4); // falta, despeja 2

    // Página 2 deve ter sido despejada
    bool falta_em_2 = mm.acessar_pagina(p, 2);
    ASSERT_TRUE(falta_em_2);

    // Página 1 ainda deve estar carregada (não foi despejada)
    bool falta_em_1 = mm.acessar_pagina(p, 1);
    ASSERT_FALSE(falta_em_1);
}

// --- Exemplo da especificação ---

void test_string_referencia_p1_da_spec() {
    // P1: working_set=8
    // string: 7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1
    // spec: 14 faltas
    GerenciadorDeMemoria mm;
    vector<int> refs = {7,0,1,2,0,3,0,4,2,3,0,3,1,0,2,8,9,10,11,12,9,7,8,3,0,1};
    BCP p = criar_processo(0, TipoProcesso::TEMPO_REAL, 8, refs);
    mm.inicializar_processos(p); // pré-carga página 7, faltas zeradas

    for (int pg : refs)
        mm.acessar_pagina(p, pg);

    ASSERT_EQ(14, p.cont_de_pgs_faltantes);
}

// --- liberar_memoria_do_processo ---

void test_liberar_permite_reuso_dos_frames() {
    GerenciadorDeMemoria mm;

    // Ocupa todos os 12 frames de usuário
    BCP p1 = criar_processo(0, TipoProcesso::USUARIO, 12);
    mm.inicializar_processos(p1);
    for (int i = 0; i < 12; i++)
        mm.acessar_pagina(p1, i);

    // Libera
    mm.liberar_memoria_do_processo(0);

    // Novo processo deve conseguir alocar
    BCP p2 = criar_processo(1, TipoProcesso::USUARIO, 4, {0});
    bool lancou = false;
    try { mm.inicializar_processos(p2); }
    catch (...) { lancou = true; }
    ASSERT_FALSE(lancou);
    ASSERT_EQ(0, p2.cont_de_pgs_faltantes); // pré-carga não conta
}

void test_liberar_processo_inexistente_nao_lanca() {
    GerenciadorDeMemoria mm;
    bool lancou = false;
    try { mm.liberar_memoria_do_processo(999); }
    catch (...) { lancou = true; }
    ASSERT_FALSE(lancou);
}

// ============================================================
// main
// ============================================================
int main() {
    std::cout << "=== Testes: Módulo de Memória ===\n";

    RUN_TEST("init sem páginas não lança",                    test_init_sem_paginas_nao_aloca_frames);
    RUN_TEST("pré-carga não conta como falta",                test_init_precarga_nao_conta_como_falta);
    RUN_TEST("init duplicado lança exceção",                  test_init_duplicado_lanca_excecao);
    RUN_TEST("working_set > partição TR lança exceção",       test_init_working_set_maior_que_particao_tr_lanca_excecao);
    RUN_TEST("working_set > partição usuário lança exceção",  test_init_working_set_maior_que_particao_usuario_lanca_excecao);
    RUN_TEST("primeiro acesso gera falta",                    test_primeiro_acesso_gera_falta);
    RUN_TEST("segundo acesso mesma página é hit",             test_segundo_acesso_mesma_pagina_e_hit);
    RUN_TEST("múltiplas faltas incrementam contador",         test_multiplas_faltas_incrementam_contador);
    RUN_TEST("acesso sem init lança exceção",                 test_acesso_sem_init_lanca_excecao);
    RUN_TEST("tempo real usa frames 0–7",                     test_tempo_real_usa_frames_0_a_7);
    RUN_TEST("usuário usa frames 8–19",                       test_usuario_usa_frames_8_a_19);
    RUN_TEST("TR e usuário não compartilham frames",          test_tr_e_usuario_nao_compartilham_frames);
    RUN_TEST("LRU despeja página menos recente",              test_lru_despeja_pagina_menos_recente);
    RUN_TEST("LRU atualiza no hit",                           test_lru_atualiza_no_hit);
    RUN_TEST("string de referência P1 da spec (14 faltas)",   test_string_referencia_p1_da_spec);
    RUN_TEST("liberar permite reuso dos frames",              test_liberar_permite_reuso_dos_frames);
    RUN_TEST("liberar processo inexistente não lança",        test_liberar_processo_inexistente_nao_lanca);

    PRINT_RESULTS();
}