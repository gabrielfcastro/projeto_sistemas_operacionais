#ifndef DESPACHANTE_HPP
#define DESPACHANTE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "filas.hpp"
#include "processo.hpp"

using namespace std;

static constexpr int QUANTUM = 1;

class Despachante {
    public:
        void carregar_arquivos(const string& arquivo_processos, const string& arquivos_paginas){
            carregar_processos(arquivo_processos);
            carregar_paginas_referenciadas(arquivos_paginas);
            validar_processos();
        }

        void executar(){
            int tempo_atual = 0;

            while(proximo_processo_a_entrar_nas_filas < (int)todos_os_processos.size() || !filas.todas_as_filas_estao_vazias()){

                atualizar_filas_com_novos_processos(tempo_atual);

                if (filas.todas_as_filas_estao_vazias()){
                    tempo_atual++;
                    continue;
                }
                filas.envelhecimento();
                BCP processo_atual = filas.proximo_processo_da_fila_a_ser_executado();
                imprimir_cabecalho_do_despachante(processo_atual);
                executar_processo(processo_atual, tempo_atual);
                tempo_atual++;
            }
        }

        const vector<BCP>& obter_processos() const { return todos_os_processos; }

    private:
        vector<BCP> todos_os_processos;
        GerenciadorDeFilas filas;
        int proximo_processo_a_entrar_nas_filas = 0;

        // 1) Abre o arquivo em modo leitura.
        void carregar_processos(const string& nome_arquivo){
            ifstream arquivo(nome_arquivo);

            if (!arquivo.is_open()){
                throw runtime_error("Não foi possível abrir: " + nome_arquivo);
            }
            // 1a) Armazena todo o conteúdo do arquivo lido
            string conteudo((istreambuf_iterator<char>(arquivo)), istreambuf_iterator<char>());
            // 1b) Filtra o conteúdo p/ estar no formato esperado
            todos_os_processos = Leitor::filtrar_e_extrair_processos(conteudo);
        }

        void carregar_paginas_referenciadas(const string& nome_arquivo){
            ifstream arquivo(nome_arquivo);

            if (!arquivo.is_open()){
                throw runtime_error("Não foi possível abrir: " + nome_arquivo);
            }
            // Buffer temporário que vai receber cada linha do arquivo
            string linha;
            int idx = 0;
            while (getline(arquivo, linha) && idx < (int)todos_os_processos.size()){
                // Ignora linhas vazias
                if (linha.find_first_not_of(" \t\r\n") == string::npos){
                    continue;
                }
                todos_os_processos[idx].lista_de_paginas_referenciadas = filtrar_paginas(linha);
                idx++;
            }
        }

        static vector<int> filtrar_paginas(const string& linha){
            vector<int> paginas;
            stringstream ss(linha);
            string token;

            while (getline(ss, token, ',')){
                size_t inicio = token.find_first_not_of(" \t");
                size_t fim    = token.find_last_not_of(" \t");

                if (inicio == string::npos){
                    continue;
                }

                paginas.push_back(stoi(token.substr(inicio, fim - inicio + 1)));
            }
            return paginas;
        }

        void validar_processos() const{
            for (int i = 0; i < (int)todos_os_processos.size(); i++){
                if (todos_os_processos[i].lista_de_paginas_referenciadas.empty()){
                    throw runtime_error("Lista de páginas de referência ausente para o processo P" + to_string(i));
                }
            }
        }

        // "Libera" a entrada dos processos na fila quando a variável tempo_atual for igual ao tempo_de_chegada
        void atualizar_filas_com_novos_processos(int tempo_atual){
            // Lidar com os vários processos que chegam simultaneamente
            while (proximo_processo_a_entrar_nas_filas < (int)todos_os_processos.size() && todos_os_processos[proximo_processo_a_entrar_nas_filas].tempo_chegada <= tempo_atual){
                filas.adicionar_processos_na_fila(todos_os_processos[proximo_processo_a_entrar_nas_filas]);
                proximo_processo_a_entrar_nas_filas++;
            }
        }

        void imprimir_cabecalho_do_despachante(const BCP& p) const{
            cout  << "dispatcher =>\n"
                  << "  PID: "      << p.id                           << "\n"
                  << "  frames: "   << p.working_set                  << "\n"
                  << "  priority: " << p.prioridade_base              << "\n"
                  << "  time: "     << p.tempo_processador_necessario << "\n"
                  << "  printers: " << p.impressora                   << "\n"
                  << "  scanners: " << p.scanner                      << "\n"
                  << "  modems: "   << p.modem                        << "\n"
                  << "  drives: "   << p.sata                         << "\n\n";
        }

        void executar_processo(BCP& processo, int& tempo_atual){
            cout << "process " << processo.id << " =>\n";
            cout << "P" << processo.id << " STARTED\n";

            if (processo.tipo == TipoProcesso::TEMPO_REAL){
                executar_ate_o_fim(processo, tempo_atual);
            }
            else{
                executar_um_quantum(processo, tempo_atual);
            }
        }

        void executar_ate_o_fim(BCP& processo, int& tempo_atual){
            while (processo.tempo_processador_restante > 0){
                processo.tempo_processador_restante--;

                int instrucao = processo.tempo_processador_necessario - processo.tempo_processador_restante;
                cout << "P" << processo.id << " instruction " << instrucao << "\n";
                tempo_atual++;
                atualizar_filas_com_novos_processos(tempo_atual);
            }
            finalizar_processo(processo);
        }

        void executar_um_quantum(BCP& processo, int& tempo_atual){
            processo.tempo_processador_restante -= QUANTUM;

            int instrucao = processo.tempo_processador_necessario - processo.tempo_processador_restante;
            cout << "P" << processo.id << " instruction " << instrucao << "\n";
            tempo_atual++;
            atualizar_filas_com_novos_processos(tempo_atual);

            if (processo.tempo_processador_restante <= 0){
                finalizar_processo(processo);
            }
            else{
                filas.excedido_o_quantum(processo);
            }
        }

        void finalizar_processo(BCP& processo){
            processo.estado_atual = EstadoProcesso::FINALIZADO;
            cout<< "P" << processo.id << " return SIGINT\n\n";
        }
};

#endif // DESPACHANTE_HPP