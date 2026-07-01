#ifndef GERENCIA_DE_MEMORIA_HPP
#define GERENCIA_DE_MEMORIA_HPP

#include <list>
#include <vector>
#include <string>
#include <stdexcept>
#include "processo.hpp"
#include <unordered_map>

using namespace std;

static constexpr int TOTAL_DE_FRAMES = 20;
static constexpr int FRAMES_TEMPO_REAL = 8;
static constexpr int FRAMES_USUARIO = 12;
static constexpr int BASE_FRAMES_TEMPO_REAL = 0;
static constexpr int BASE_FRAMES_USUARIO = 8;

struct FrameDaMemoria{
    bool ocupado;
    int id_processo;
    int pagina_logica;

    // Todo frame nasce livre e sem "dono"
    FrameDaMemoria() : ocupado(false), id_processo(-1), pagina_logica(-1) {}
};

struct ContextoMemoriaProcesso{
    int id_processo;
    int limite_de_frames;
    list<int> ordem_lru;
    unordered_map<int, int> tabela_de_paginas;

    ContextoMemoriaProcesso() : id_processo(-1), limite_de_frames(0) {}
};

class GerenciadorDeMemoria{
    public:
        GerenciadorDeMemoria(){
            frames.resize(TOTAL_DE_FRAMES); // Constroi a memória com 20 frames
        }

        void inicializar_processos(BCP& processo){
            if (contextos.count(processo.id)){
                throw runtime_error("Processo P" + to_string(processo.id) + " já foi inicializado na memória.");
            }

            // Se pedir mais frames do que o total
            if (processo.tipo == TipoProcesso::TEMPO_REAL && processo.working_set > FRAMES_TEMPO_REAL){
                throw runtime_error("Processo TR P" + to_string(processo.id) + " pede mais frames do que a partição suporta.");
            }
            if (processo.tipo == TipoProcesso::USUARIO && processo.working_set > FRAMES_USUARIO){
                throw runtime_error("Processo de Usuário P" + to_string(processo.id) + " pede mais frames do que a partição suporta.");
            }

            // Cria uma tabela de páginas virtuais p/ cada processo
            ContextoMemoriaProcesso contexto;
            contexto.id_processo = processo.id;
            contexto.limite_de_frames = processo.working_set;
            contextos[processo.id] = contexto;

            // Não contabiliza a primeira página
            if (!processo.lista_de_paginas_referenciadas.empty()){
                acessar_pagina(processo, processo.lista_de_paginas_referenciadas[0]);
                processo.cont_de_pgs_faltantes = 0;
            }
        }

        bool acessar_pagina(BCP &processo, int pagina){
            auto& contexto = obter_contexto(processo.id);

            // Verifica se a página solicitada está na tabela de páginas
            if (contexto.tabela_de_paginas.count(pagina)){
                atualizar_uso_recente_lru(contexto, pagina);
                return false;
            }

            processo.cont_de_pgs_faltantes++;

            // Aloca na RAM a página
            int frame_fisico_alocado = alocar_frame(processo, contexto);

            contexto.tabela_de_paginas[pagina] = frame_fisico_alocado;
            // Coloca essa página no topo do ranking do LRU
            contexto.ordem_lru.push_front(pagina);

            // Atualiza a RAM
            frames[frame_fisico_alocado].ocupado = true;
            frames[frame_fisico_alocado].id_processo = processo.id;
            frames[frame_fisico_alocado].pagina_logica = pagina;

            return true;
        }

        void liberar_memoria_do_processo(int id_processo){
            if (!contextos.count(id_processo)){
                return;
            }

            // Varre a tabela do processo "morto"
            auto& contexto = contextos[id_processo];
            for (auto& par : contexto.tabela_de_paginas){
                int frame = par.second;
                frames[frame] = FrameDaMemoria(); // Zera o dono, desmarca ocupado...
            }
            // Apaga a tabela lógica do processo encerrado
            contextos.erase(id_processo);
        }

    private:
        vector<FrameDaMemoria> frames;
        unordered_map<int, ContextoMemoriaProcesso> contextos;

        ContextoMemoriaProcesso& obter_contexto(int id_processo){
            auto it = contextos.find(id_processo);
            if (it == contextos.end()){
                throw runtime_error("Processo P" + to_string(id_processo) + " não inicializado na memória.");
            }
            return it->second;
        }

        void definir_limites_da_particao(const BCP& processo, int& frame_inicial, int& limite_de_frames) const{
            if (processo.tipo == TipoProcesso::TEMPO_REAL){
                frame_inicial = BASE_FRAMES_TEMPO_REAL;
                limite_de_frames = BASE_FRAMES_TEMPO_REAL + FRAMES_TEMPO_REAL;
            } else {
                frame_inicial = BASE_FRAMES_USUARIO;
                limite_de_frames = BASE_FRAMES_USUARIO + FRAMES_USUARIO;
            }
        }

        int alocar_frame(const BCP& processo, ContextoMemoriaProcesso& contexto){
            int frame_inicial, limite_de_frames;
            definir_limites_da_particao(processo, frame_inicial, limite_de_frames);

            // O processo ainda não estourou o Working Set
            if ((int)contexto.tabela_de_paginas.size() < contexto.limite_de_frames){
                for (int i = frame_inicial; i < limite_de_frames; i++) {
                    if (!frames[i].ocupado){
                        return i;
                    }
                }
            }
            // O processo bateu o limite do Working Set ou a partição lotou
            return evocar_lru_e_liberar_frame(processo, contexto);
        }

        int evocar_lru_e_liberar_frame(const BCP& processo, ContextoMemoriaProcesso& contexto){
            if (contexto.ordem_lru.empty()){
                throw runtime_error("LRU quebrado: Nenhuma página para substituir no P" + to_string(processo.id));
            }

            int pagina_lru = contexto.ordem_lru.back();
            contexto.ordem_lru.pop_back();

            int frame_liberado = contexto.tabela_de_paginas.at(pagina_lru);
            contexto.tabela_de_paginas.erase(pagina_lru);

            return frame_liberado;
        }

        void atualizar_uso_recente_lru(ContextoMemoriaProcesso& contexto, int pagina){
            contexto.ordem_lru.remove(pagina);
            contexto.ordem_lru.push_front(pagina);
        }

};

#endif // GERENCIA_DE_MEMORIA_HPP