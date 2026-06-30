#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <string>
#include "processo.hpp"

using namespace std;

// =============================================================================
// Módulo de Recursos (Gerenciador de E/S) — Pessoa C
//
// Administra a alocação e liberação dos dispositivos de E/S do pseudo-SO,
// garantindo uso exclusivo de cada unidade (um processo por vez) e sem
// preempção. Processos de tempo real NÃO utilizam recursos de E/S.
//
// Recursos disponíveis (Seção 1.3 da especificação):
//   - 1 scanner
//   - 2 impressoras
//   - 1 modem
//   - 2 dispositivos SATA
// =============================================================================

// Quantidade de unidades de cada recurso disponível no sistema.
static constexpr int QTD_SCANNERS    = 1;
static constexpr int QTD_IMPRESSORAS = 2;
static constexpr int QTD_MODEMS      = 1;
static constexpr int QTD_SATA        = 2;

// -----------------------------------------------------------------------------
// Semáforo contável compatível com C++17.
//
// std::counting_semaphore só existe a partir do C++20; como o projeto compila
// com -std=c++17, implementamos a primitiva com mutex + variável de condição.
// Serve como mecanismo de sincronização de baixo nível para o controle de
// acesso exclusivo aos recursos.
// -----------------------------------------------------------------------------
class Semaforo {
public:
    explicit Semaforo(int valor_inicial) : contador(valor_inicial) {
        if (valor_inicial < 0) {
            throw invalid_argument("Semáforo não pode iniciar com valor negativo.");
        }
    }

    // P / wait — bloqueia enquanto não houver unidade disponível.
    void adquirir() {
        unique_lock<mutex> trava(mtx);
        cond.wait(trava, [this] { return contador > 0; });
        contador--;
    }

    // Tentativa não bloqueante de aquisição. Retorna true se conseguiu a unidade.
    bool tentar_adquirir() {
        lock_guard<mutex> trava(mtx);
        if (contador > 0) {
            contador--;
            return true;
        }
        return false;
    }

    // V / signal — devolve uma unidade e acorda um processo em espera.
    void liberar() {
        lock_guard<mutex> trava(mtx);
        contador++;
        cond.notify_one();
    }

    int disponivel() const {
        lock_guard<mutex> trava(mtx);
        return contador;
    }

private:
    mutable mutex      mtx;
    condition_variable cond;
    int                contador;
};

// -----------------------------------------------------------------------------
// Resultado de uma tentativa de alocação de recursos.
// -----------------------------------------------------------------------------
struct ResultadoAlocacao {
    bool   sucesso;
    string mensagem;
};

// -----------------------------------------------------------------------------
// Gerenciador de Recursos de E/S.
//
// Estratégia de alocação: tudo-ou-nada e não bloqueante (tentar_alocar). O
// processo só "entra" se conseguir, de uma vez, TODOS os recursos que pediu.
// Caso falte algum, os que já foram obtidos são devolvidos. Essa abordagem
// evita deadlock (um processo nunca fica retendo um recurso à espera de outro)
// e respeita a regra de não haver preempção na alocação dos dispositivos.
// -----------------------------------------------------------------------------
class GerenciadorDeRecursos {
public:
    GerenciadorDeRecursos()
        : scanner(QTD_SCANNERS),
          impressora(QTD_IMPRESSORAS),
          modem(QTD_MODEMS),
          sata(QTD_SATA)
    {}

    // Tenta alocar, de forma atômica, todos os recursos solicitados pelo
    // processo. Processos de tempo real não usam E/S, portanto são sempre
    // liberados para executar sem reter nenhum dispositivo.
    //
    // FLUXO (chamado pelo dispatcher antes de executar um processo de usuário):
    //   1. Se for tempo real -> não usa E/S, retorna sucesso sem reter nada.
    //   2. Tenta pegar, na ordem, cada recurso que o processo pediu (campo > 0).
    //   3. Se faltar QUALQUER recurso -> desfaz (rollback) os já obtidos e falha.
    //   4. Se conseguiu todos -> sucesso (o processo pode executar).
    // Esse "tudo-ou-nada" evita deadlock: nunca seguramos um recurso esperando outro.
    ResultadoAlocacao tentar_alocar(const BCP& processo) {
        // Passo 1: tempo real não disputa recursos de E/S.
        if (processo.tipo == TipoProcesso::TEMPO_REAL) {
            return { true, "Processo de tempo real não utiliza recursos de E/S." };
        }

        // Marcadores do que já foi adquirido — necessários para o rollback (passo 3).
        bool obteve_scanner    = false;
        bool obteve_impressora = false;
        bool obteve_modem      = false;
        bool obteve_sata       = false;

        // Passo 2: tenta cada recurso pedido. Ao falhar, devolve o que já pegou.
        if (processo.scanner > 0) {
            obteve_scanner = scanner.tentar_adquirir();
            if (!obteve_scanner) {
                return falhar("scanner");
            }
        }
        if (processo.impressora > 0) {
            obteve_impressora = impressora.tentar_adquirir();
            if (!obteve_impressora) {
                desfazer(obteve_scanner, false, false, false);
                return falhar("impressora");
            }
        }
        if (processo.modem > 0) {
            obteve_modem = modem.tentar_adquirir();
            if (!obteve_modem) {
                desfazer(obteve_scanner, obteve_impressora, false, false);
                return falhar("modem");
            }
        }
        if (processo.sata > 0) {
            obteve_sata = sata.tentar_adquirir();
            if (!obteve_sata) {
                desfazer(obteve_scanner, obteve_impressora, obteve_modem, false);
                return falhar("dispositivo SATA");
            }
        }

        // Passo 4: todos os recursos pedidos foram obtidos com exclusividade.
        return { true, "Recursos alocados com sucesso para o processo "
                       + to_string(processo.id) + "." };
    }

    // Devolve ao sistema todos os recursos que o processo havia solicitado.
    void liberar(const BCP& processo) {
        if (processo.tipo == TipoProcesso::TEMPO_REAL) {
            return;
        }
        desfazer(processo.scanner    > 0,
                 processo.impressora > 0,
                 processo.modem      > 0,
                 processo.sata       > 0);
    }

    // Consultas de disponibilidade (úteis para testes e para a fila "global").
    int scanners_disponiveis()    const { return scanner.disponivel(); }
    int impressoras_disponiveis() const { return impressora.disponivel(); }
    int modems_disponiveis()      const { return modem.disponivel(); }
    int sata_disponiveis()        const { return sata.disponivel(); }

private:
    Semaforo scanner;
    Semaforo impressora;
    Semaforo modem;
    Semaforo sata;

    // Libera seletivamente os recursos marcados como obtidos.
    void desfazer(bool tinha_scanner, bool tinha_impressora,
                  bool tinha_modem, bool tinha_sata) {
        if (tinha_scanner)    scanner.liberar();
        if (tinha_impressora) impressora.liberar();
        if (tinha_modem)      modem.liberar();
        if (tinha_sata)       sata.liberar();
    }

    static ResultadoAlocacao falhar(const string& recurso) {
        return { false, "Recurso indisponível: " + recurso + "." };
    }
};

#endif // RESOURCE_HPP
