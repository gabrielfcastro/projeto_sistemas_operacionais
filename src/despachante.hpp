#ifndef DESPACHANTE_HPP
#define DESPACHANTE_HPP

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include "filas.hpp"
#include "processo.hpp"
#include "memoria.hpp"
#include "resource.hpp"
#include "filesystem.hpp"

using namespace std;

static constexpr int QUANTUM = 1;

class Despachante {
    public:
        void carregar_arquivos(const string& arquivo_processos,
                                const string& arquivo_paginas,
                                const string& arquivo_files = ""){
            carregar_processos(arquivo_processos);
            carregar_paginas_referenciadas(arquivo_paginas);
            validar_processos();

            if (!arquivo_files.empty()){
                carregar_conteudo_files(arquivo_files);
            }
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

                if (processo_atual.tipo == TipoProcesso::USUARIO){
                    ResultadoAlocacao r = recursos.tentar_alocar(processo_atual);
                    if (!r.sucesso){
                        processo_atual.estado_atual = EstadoProcesso::PRONTO;
                        filas.excedido_o_quantum(processo_atual);
                        continue;
                    }
                }

                garantir_memoria_inicializada(processo_atual);

                if (!processos_ja_impressos.count(processo_atual.id)){
                    imprimir_cabecalho_do_despachante(processo_atual);
                }

                executar_processo(processo_atual, tempo_atual);
                tempo_atual++;
            }

            if (tem_sistema_de_arquivos){
                processar_sistema_de_arquivos();
            }

            imprimir_faltas_de_pagina();
        }

        const vector<BCP>& obter_processos() const { return todos_os_processos; }

    private:
        vector<BCP> todos_os_processos;
        GerenciadorDeFilas filas;
        GerenciadorDeMemoria memoria;
        GerenciadorDeRecursos recursos;
        int proximo_processo_a_entrar_nas_filas = 0;

        unordered_set<int> processos_ja_inicializados_na_memoria;
        unordered_set<int> processos_ja_impressos;

        string conteudo_files;
        bool   tem_sistema_de_arquivos = false;

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

        void carregar_conteudo_files(const string& nome_arquivo){
            ifstream arquivo(nome_arquivo);
            if (!arquivo.is_open()){
                throw runtime_error("Não foi possível abrir: " + nome_arquivo);
            }
            conteudo_files.assign((istreambuf_iterator<char>(arquivo)),
                                   istreambuf_iterator<char>());
            tem_sistema_de_arquivos = true;
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

        void garantir_memoria_inicializada(BCP& processo){
            if (processos_ja_inicializados_na_memoria.count(processo.id)){
                return;
            }
            memoria.inicializar_processos(processo); // pré-carga da página 0
            for (size_t i = 1; i < processo.lista_de_paginas_referenciadas.size(); i++){
                memoria.acessar_pagina(processo, processo.lista_de_paginas_referenciadas[i]);
            }
            processos_ja_inicializados_na_memoria.insert(processo.id);
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

            // Só imprime a indicação de START se for a primeira vez na CPU
            if (!processos_ja_impressos.count(processo.id)){
                cout << "process " << processo.id << " =>\n";
                cout << "P" << processo.id << " STARTED\n";
                processos_ja_impressos.insert(processo.id);
            }

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

            recursos.liberar(processo);

            if (processo.tempo_processador_restante <= 0){
                finalizar_processo(processo);
            }
            else{
                sincronizar_estatisticas(processo);
                filas.excedido_o_quantum(processo);
            }
        }

        void finalizar_processo(BCP& processo){
            processo.estado_atual = EstadoProcesso::FINALIZADO;
            cout<< "P" << processo.id << " return SIGINT\n\n";
            memoria.liberar_memoria_do_processo(processo.id);
            sincronizar_estatisticas(processo);
        }

        void sincronizar_estatisticas(const BCP& processo){
            if (processo.id < 0 || processo.id >= (int)todos_os_processos.size()){
                return;
            }
            todos_os_processos[processo.id].cont_de_pgs_faltantes =
                processo.cont_de_pgs_faltantes;
        }

        void processar_sistema_de_arquivos(){
            InfoProcessos info;
            info.eh_tempo_real.reserve(todos_os_processos.size());
            for (const BCP& p : todos_os_processos){
                info.eh_tempo_real.push_back(p.tipo == TipoProcesso::TEMPO_REAL);
            }
            cout << LeitorArquivos::processar(conteudo_files, info);
        }

        void imprimir_faltas_de_pagina() const{
            cout << "Número de Faltas de Páginas por processo:\n";
            for (const BCP& p : todos_os_processos){
                cout << "P" << p.id << " = " << p.cont_de_pgs_faltantes
                     << " faltas de páginas\n";
            }
        }
};

#endif // DESPACHANTE_HPP