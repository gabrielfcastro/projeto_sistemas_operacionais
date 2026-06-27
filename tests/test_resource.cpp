#include "test_framework.hpp"
#include "../src/resource.hpp"
#include <iostream>

// Cria um BCP de usuário pedindo os recursos indicados.
static BCP processo_usuario(int id, int impressora, int scanner,
                            int modem, int sata) {
    BCP p;
    p.id         = id;
    p.tipo       = TipoProcesso::USUARIO;
    p.impressora = impressora;
    p.scanner    = scanner;
    p.modem      = modem;
    p.sata       = sata;
    return p;
}

static BCP processo_tempo_real(int id) {
    BCP p;
    p.id   = id;
    p.tipo = TipoProcesso::TEMPO_REAL;
    return p;
}

// O semáforo deve respeitar o valor inicial e bloquear/contabilizar unidades.
static void test_semaforo_basico() {
    Semaforo sem(2);
    ASSERT_EQ(2, sem.disponivel());

    ASSERT_TRUE(sem.tentar_adquirir());
    ASSERT_TRUE(sem.tentar_adquirir());
    ASSERT_EQ(0, sem.disponivel());

    // Sem unidades, a tentativa não bloqueante deve falhar.
    ASSERT_FALSE(sem.tentar_adquirir());

    sem.liberar();
    ASSERT_EQ(1, sem.disponivel());
    ASSERT_TRUE(sem.tentar_adquirir());
}

// Processos de tempo real não consomem recursos de E/S.
static void test_tempo_real_nao_usa_recursos() {
    GerenciadorDeRecursos gerenciador;
    BCP tr = processo_tempo_real(0);

    ResultadoAlocacao resultado = gerenciador.tentar_alocar(tr);
    ASSERT_TRUE(resultado.sucesso);

    // Nenhuma unidade deve ter sido consumida.
    ASSERT_EQ(QTD_SCANNERS,    gerenciador.scanners_disponiveis());
    ASSERT_EQ(QTD_IMPRESSORAS, gerenciador.impressoras_disponiveis());
    ASSERT_EQ(QTD_MODEMS,      gerenciador.modems_disponiveis());
    ASSERT_EQ(QTD_SATA,        gerenciador.sata_disponiveis());
}

// Alocação e liberação devolvem corretamente as unidades ao sistema.
static void test_alocar_e_liberar() {
    GerenciadorDeRecursos gerenciador;
    BCP p = processo_usuario(1, /*impressora*/1, /*scanner*/1,
                             /*modem*/0, /*sata*/1);

    ResultadoAlocacao resultado = gerenciador.tentar_alocar(p);
    ASSERT_TRUE(resultado.sucesso);
    ASSERT_EQ(0, gerenciador.scanners_disponiveis());
    ASSERT_EQ(1, gerenciador.impressoras_disponiveis());
    ASSERT_EQ(1, gerenciador.modems_disponiveis());
    ASSERT_EQ(1, gerenciador.sata_disponiveis());

    gerenciador.liberar(p);
    ASSERT_EQ(QTD_SCANNERS,    gerenciador.scanners_disponiveis());
    ASSERT_EQ(QTD_IMPRESSORAS, gerenciador.impressoras_disponiveis());
    ASSERT_EQ(QTD_MODEMS,      gerenciador.modems_disponiveis());
    ASSERT_EQ(QTD_SATA,        gerenciador.sata_disponiveis());
}

// Uso exclusivo: o único scanner não pode ser alocado a dois processos.
static void test_uso_exclusivo_scanner() {
    GerenciadorDeRecursos gerenciador;
    BCP a = processo_usuario(1, 0, 1, 0, 0);
    BCP b = processo_usuario(2, 0, 1, 0, 0);

    ASSERT_TRUE(gerenciador.tentar_alocar(a).sucesso);
    ASSERT_FALSE(gerenciador.tentar_alocar(b).sucesso);

    // Depois que A libera, B consegue.
    gerenciador.liberar(a);
    ASSERT_TRUE(gerenciador.tentar_alocar(b).sucesso);
}

// Estratégia tudo-ou-nada: se faltar um recurso, nenhum é retido (rollback).
static void test_rollback_tudo_ou_nada() {
    GerenciadorDeRecursos gerenciador;

    // Ocupa o único modem.
    BCP dono_do_modem = processo_usuario(1, 0, 0, 1, 0);
    ASSERT_TRUE(gerenciador.tentar_alocar(dono_do_modem).sucesso);

    // Este processo pede impressora + modem; o modem está ocupado, então a
    // alocação inteira deve falhar e a impressora NÃO pode ficar retida.
    BCP p = processo_usuario(2, 1, 0, 1, 0);
    ASSERT_FALSE(gerenciador.tentar_alocar(p).sucesso);
    ASSERT_EQ(QTD_IMPRESSORAS, gerenciador.impressoras_disponiveis());
}

int main() {
    RUN_TEST("Semáforo básico",                 test_semaforo_basico);
    RUN_TEST("Tempo real não usa recursos",     test_tempo_real_nao_usa_recursos);
    RUN_TEST("Alocar e liberar recursos",       test_alocar_e_liberar);
    RUN_TEST("Uso exclusivo do scanner",        test_uso_exclusivo_scanner);
    RUN_TEST("Rollback tudo-ou-nada",           test_rollback_tudo_ou_nada);
    PRINT_RESULTS();
}
