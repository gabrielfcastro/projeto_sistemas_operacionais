#ifndef FILAS_HPP
#define FILAS_HPP

#include <queue>
#include <array>
#include <vector>
#include <stdexcept>
#include "processo.hpp"

using namespace std;

static constexpr int TEMPO_MAXIMO_DE_ESPERA = 5;
static constexpr int MAXIMO_DE_PROCESSOS = 1000;
static constexpr int QUANTIDADE_FILAS_USUARIO = 3;

class GerenciadorDeFilas {
public:
    GerenciadorDeFilas() : total_de_processos(0) {}

    void processo_a_ser_executado(BCP& processo){
        if (total_de_processos >= MAXIMO_DE_PROCESSOS){
            throw overflow_error("Capacidade máxima de processos atingida.");
        }

        processo.estado_atual = EstadoProcesso::PRONTO;

        // Fila de Tempo Real são prioritárias (FIFO)
        if (processo.tipo == TipoProcesso::TEMPO_REAL) {
            fila_tempo_real.push(processo);
        } else {
            int fila = determinar_fila_baseado_na_prioridade(processo.prioridade_dinamica);
            filas_de_usuario[fila].push(processo);
        }

        total_de_processos++;
    }

    BCP proximo_processo_a_ser_executado() {
        // 1 - Verifica a fila de processos em tempo real
        if (!fila_tempo_real.empty()){
            BCP processo_escolhido = fila_tempo_real.front();
            fila_tempo_real.pop();
            total_de_processos--;
            processo_escolhido.estado_atual = EstadoProcesso::EXECUTANDO;
            return processo_escolhido;
        }

        for (int i = 0; i < QUANTIDADE_FILAS_USUARIO; i++){
            // Garante que não tenho num processo na fila de processo em tempo real
            if (!filas_de_usuario[i].empty()){
                // Pega o primeiro processo que está na fila de prioridade, seguindo a ordem 1,2 e 3
                BCP processo_escolhido = filas_de_usuario[i].front();
                filas_de_usuario[i].pop();
                total_de_processos--;
                processo_escolhido.estado_atual = EstadoProcesso::EXECUTANDO;
                return processo_escolhido;
            }
        }

        throw underflow_error("Todas as filas estão vazias.");
    }

    void envelhecimento(){
        for (int nivel = 0; nivel < QUANTIDADE_FILAS_USUARIO; nivel++){
            int num_de_processos_na_fila = static_cast<int>(filas_de_usuario[nivel].size());

            for (int j = 0; j < num_de_processos_na_fila; j++){
                BCP proc = filas_de_usuario[nivel].front();
                filas_de_usuario[nivel].pop();
                proc.tempo_esperando++;

                // 1 - Mudo a prioridade do meu processo e consequentemente a fila que ele vai estar
                if (proc.tempo_esperando >= TEMPO_MAXIMO_DE_ESPERA && proc.prioridade_dinamica > 1){
                    proc.prioridade_dinamica--;
                    proc.tempo_esperando = 0;

                    int fila_nova = determinar_fila_baseado_na_prioridade(proc.prioridade_dinamica);
                    filas_de_usuario[fila_nova].push(proc);
                }
                // 2 - Se não simplesmente volta p/ final da fila p/ esperar mais
                else{
                    filas_de_usuario[nivel].push(proc);
                }
            }
        }
    }

    void excedido_o_quantum(BCP& processo){
        processo.tempo_esperando = 0;
        processo.estado_atual = EstadoProcesso::PRONTO;

        int fila = determinar_fila_baseado_na_prioridade(processo.prioridade_dinamica);
        filas_de_usuario[fila].push(processo);
        total_de_processos++;
    }

    bool todas_as_filas_estao_vazias() const{
        if (!fila_tempo_real.empty()){
            return false;
        }
        for (const auto& fila : filas_de_usuario){
            if (!fila.empty()){
                return false;
            }
        }
        return true;
    }

    bool fila_tempo_real_esta_vazia() const {
        return fila_tempo_real.empty();
    }

    bool filas_usuario_estao_vazias() const {
        for (const auto& fila : filas_de_usuario){
            if (!fila.empty()){
                return false;
            }
        }
        return true;
    }

    int tamanho_fila_tempo_real() const {
        return static_cast<int>(fila_tempo_real.size());
    }

    int tamanho_fila_usuario(int prioridade) const {
        return static_cast<int>(filas_de_usuario[determinar_fila_baseado_na_prioridade(prioridade)].size());
    }

    int quantidade_total_processos() const {
        return total_de_processos;
    }

    // Retorna todos os processos de uma fila de usuário
    vector<BCP> processos_da_fila_de_usuario(int prioridade) const {
        queue<BCP> copia = filas_de_usuario[determinar_fila_baseado_na_prioridade(prioridade)];
        vector<BCP> resultado;
        while(!copia.empty()){
            resultado.push_back(copia.front());
            copia.pop();
        }
        return resultado;
    }

private:
    queue<BCP> fila_tempo_real;
    array<queue<BCP>, QUANTIDADE_FILAS_USUARIO> filas_de_usuario;
    int total_de_processos;

    // Converte a prioridade em índice p/ serem usados na hora de organizar a fila
    static int determinar_fila_baseado_na_prioridade(int prioridade){
        if (prioridade < 1 || prioridade > 3){
            throw invalid_argument("Prioridade de usuário inválida: " + to_string(prioridade));
        }
        return prioridade - 1;
    }

};

#endif // FILAS_HPP