#ifndef PROCESSO_HPP
#define PROCESSO_HPP

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

using namespace std;

enum class TipoProcesso {
    TEMPO_REAL,     // Prioridade 0
    USUARIO         // Prioridades 1, 2 e 3
};

enum class EstadoProcesso {
    NOVO,
    PRONTO,
    EXECUTANDO,
    BLOQUEADO,
    FINALIZADO
};

// Bloco de Controle de Processo
struct BCP {
    int id;
    int prioridade_base;
    int prioridade_dinamica;
    TipoProcesso tipo;
    EstadoProcesso estado_atual;

    int tempo_chegada;                  // Momento em que o processo chega ao sistema
    int tempo_processador_necessario;   // Tempo de CPU necessário para o processo ser concluído
    int tempo_processador_restante;

    int working_set;
    int cont_de_pgs_faltantes;
    vector<int> tabela_de_paginas;
    vector<int> lista_de_paginas_referenciadas;

    int tempo_esperando;

    int impressora;
    int scanner;
    int modem;
    int sata;

    BCP()
        :
        id(-1), prioridade_base(0), prioridade_dinamica(0),
        tipo(TipoProcesso::USUARIO), estado_atual(EstadoProcesso::NOVO),
        tempo_chegada(0), tempo_processador_necessario(0), tempo_processador_restante(0),
        working_set(0), cont_de_pgs_faltantes(0), tempo_esperando(0),
        impressora(0), scanner(0), modem(0), sata(0)
    {}
};

// Formato esperado: <tempo_chegada>, <prioridade>, <tempo_processador>, <tamanho_conjunto_trabalho>, <impressora>, <scanner>, <modem>, <sata>
namespace Leitor {

    inline BCP ler_linha(const string& linha, int id_processo){
        BCP proc;
        proc.id = id_processo;

        stringstream linha_antes_de_ser_filtrada(linha);
        string valores;
        vector<int> valores_filtrados;

        while(getline(linha_antes_de_ser_filtrada, valores, ',')){
            size_t inicio = valores.find_first_not_of(" \t");
            size_t fim = valores.find_last_not_of(" \t");

            if (inicio == string::npos){
                throw invalid_argument("Campo vazio detectado na linha: " + linha);
            }

            valores = valores.substr(inicio, fim - inicio + 1);
            valores_filtrados.push_back(stoi(valores));
        }

        if (valores_filtrados.size() != 8){
            throw invalid_argument("Quantidade incorreta de campos (esperado 8) na linha: " + linha);
        }

        proc.tempo_chegada                  = valores_filtrados[0];

        proc.prioridade_base                = valores_filtrados[1];
        proc.prioridade_dinamica            = valores_filtrados[1];

        proc.tempo_processador_necessario   = valores_filtrados[2];
        proc.tempo_processador_restante     = valores_filtrados[2];

        proc.working_set                    = valores_filtrados[3];
        proc.impressora                     = valores_filtrados[4];
        proc.scanner                        = valores_filtrados[5];
        proc.modem                          = valores_filtrados[6];
        proc.sata                           = valores_filtrados[7];

        if (proc.prioridade_base < 0 || proc.prioridade_base > 3){
            throw invalid_argument("Prioridade fora do limite (0 a 3): " + to_string(proc.prioridade_base));
        }

        proc.tipo = (proc.prioridade_base == 0) ? TipoProcesso::TEMPO_REAL : TipoProcesso::USUARIO;
        proc.estado_atual = EstadoProcesso::NOVO;

        // Processos de tempo real não precisam de recursos de E/S
        if (proc.tipo == TipoProcesso::TEMPO_REAL){
            proc.impressora = 0;
            proc.scanner    = 0;
            proc.modem      = 0;
            proc.sata       = 0;
        }

        proc.tabela_de_paginas.clear();
        proc.cont_de_pgs_faltantes = 0;

        return proc;
    }

    // Lê o arquivo e retorna um vetor de BCPs
    inline vector<BCP> carregar_arquivo_processos(const string& arquivo_filtrado){
        vector<BCP> lista_de_processos;
        stringstream arquivo_antes_de_ser_filtrado(arquivo_filtrado);
        string linha_atual;
        int id_sequencial = 0;

        while (getline(arquivo_antes_de_ser_filtrado, linha_atual)){

            if (linha_atual.find_first_not_of(" \t\r\n") == string::npos)
                continue;

            // Chama a função atualizada ler_linha
            lista_de_processos.push_back(ler_linha(linha_atual, id_sequencial++));
        }

        return lista_de_processos; // Corrigido com ponto e vírgula
    }
}

#endif // PROCESSO_HPP