#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

using namespace std;

// =============================================================================
// Módulo de Arquivos (Sistema de Arquivos) — Pessoa C
//
// Permite que os processos criem e deletem arquivos no disco usando alocação
// contígua com a política first-fit (busca sempre a partir do primeiro bloco
// do disco). Um arquivo é tratado como uma unidade de manipulação e seus dados
// permanecem no disco mesmo após o término do processo que o criou.
//
// Regras de permissão (Seção 1.4 da especificação):
//   - Processos de TEMPO REAL podem criar (se houver espaço) e deletar
//     QUALQUER arquivo, mesmo que não tenham sido criados por eles.
//   - Processos de USUÁRIO podem criar quantos arquivos quiserem (se houver
//     espaço), mas só podem deletar arquivos criados por eles mesmos.
//
// Este módulo é independente dos demais: para decidir permissões ele só precisa
// saber se um processo existe e se ele é de tempo real (informação derivada do
// PCB, repassada via InfoProcessos).
// =============================================================================

// Marca de bloco livre no disco.
static constexpr char BLOCO_LIVRE = '\0';

// -----------------------------------------------------------------------------
// Informações mínimas sobre os processos necessárias ao sistema de arquivos.
//
// O índice do vetor é o PID do processo (que sempre começa em 0 e vai até a
// quantidade de processos - 1). O tamanho do vetor define quantos processos
// existem; um PID é válido apenas se estiver dentro desse intervalo.
// -----------------------------------------------------------------------------
struct InfoProcessos {
    vector<bool> eh_tempo_real;

    bool existe(int pid) const {
        return pid >= 0 && pid < static_cast<int>(eh_tempo_real.size());
    }

    bool tempo_real(int pid) const {
        return existe(pid) && eh_tempo_real[pid];
    }
};

// -----------------------------------------------------------------------------
// Representação de um arquivo gravado no disco (alocação contígua).
// -----------------------------------------------------------------------------
struct Arquivo {
    char nome;            // Identificador do arquivo (uma letra).
    int  bloco_inicial;   // Primeiro bloco ocupado.
    int  quantidade_blocos;
    int  dono;            // PID de quem criou; -1 para arquivos pré-existentes.
};

// Resultado da execução de uma operação do sistema de arquivos.
struct ResultadoOperacao {
    bool   sucesso;
    string mensagem;
};

// Códigos das operações lidas do arquivo de entrada.
enum class CodigoOperacao {
    CRIAR   = 0,
    DELETAR = 1
};

// Operação a ser executada pelo sistema de arquivos.
struct OperacaoArquivo {
    int            pid;
    CodigoOperacao codigo;
    char           nome;
    int            quantidade_blocos;  // Usado apenas em CRIAR.
};

// -----------------------------------------------------------------------------
// Sistema de Arquivos.
// -----------------------------------------------------------------------------
class SistemaDeArquivos {
public:
    explicit SistemaDeArquivos(int total_blocos)
        : disco(total_blocos, BLOCO_LIVRE)
    {
        if (total_blocos <= 0) {
            throw invalid_argument("O disco precisa ter ao menos um bloco.");
        }
    }

    // Registra um arquivo que já está gravado no disco antes da execução das
    // operações (dono = -1, pois não foi criado por nenhum processo).
    void adicionar_arquivo_inicial(char nome, int bloco_inicial, int quantidade) {
        if (bloco_inicial < 0 || quantidade <= 0 ||
            bloco_inicial + quantidade > static_cast<int>(disco.size())) {
            throw invalid_argument("Arquivo inicial fora dos limites do disco.");
        }
        ocupar_blocos(bloco_inicial, quantidade, nome);
        arquivos.push_back({ nome, bloco_inicial, quantidade, -1 });
    }

    // Tenta criar um arquivo usando first-fit. Qualquer processo existente pode
    // criar, desde que haja espaço contíguo suficiente.
    //
    // SEQUÊNCIA DE VERIFICAÇÕES (cada uma pode falhar e encerrar a operação):
    //   1. O processo (PID) existe?
    //   2. Já existe arquivo com esse nome?
    //   3. A quantidade de blocos pedida é válida?
    //   4. Existe espaço contíguo (first-fit)? Se sim, grava o arquivo.
    ResultadoOperacao criar(int pid, const InfoProcessos& info,
                            char nome, int quantidade_blocos) {
        // 1. PID precisa estar dentro de [0, quantidade de processos).
        if (!info.existe(pid)) {
            return { false, "O processo " + to_string(pid) + " não existe." };
        }
        // 2. Nomes de arquivo são únicos no disco.
        if (buscar_arquivo(nome) != nullptr) {
            return { false, prefixo_processo(pid) + " não pode criar o arquivo "
                            + nome + " (arquivo já existe)." };
        }
        // 3. Não faz sentido criar arquivo de tamanho zero/negativo.
        if (quantidade_blocos <= 0) {
            return { false, prefixo_processo(pid) + " não pode criar o arquivo "
                            + nome + " (quantidade de blocos inválida)." };
        }

        // 4. first-fit procura a primeira sequência contígua livre suficiente.
        int bloco_inicial = encontrar_first_fit(quantidade_blocos);
        if (bloco_inicial < 0) {
            return { false, prefixo_processo(pid) + " não pode criar o arquivo "
                            + nome + " (falta de espaço)." };
        }

        // Grava: marca os blocos no disco e registra o arquivo (dono = pid).
        ocupar_blocos(bloco_inicial, quantidade_blocos, nome);
        arquivos.push_back({ nome, bloco_inicial, quantidade_blocos, pid });

        return { true, prefixo_processo(pid) + " criou o arquivo " + nome + " ("
                       + descrever_blocos(bloco_inicial, quantidade_blocos)
                       + ")." };
    }

    // Tenta deletar um arquivo, respeitando as regras de permissão.
    //
    // SEQUÊNCIA DE VERIFICAÇÕES:
    //   1. O processo (PID) existe?
    //   2. O arquivo existe no disco?
    //   3. Permissão: tempo real deleta qualquer um; usuário só os que criou.
    //   4. Apaga: libera os blocos e remove o registro do arquivo.
    ResultadoOperacao deletar(int pid, const InfoProcessos& info, char nome) {
        // 1. PID válido?
        if (!info.existe(pid)) {
            return { false, "O processo " + to_string(pid) + " não existe." };
        }

        // 2. Arquivo existe?
        Arquivo* arquivo = buscar_arquivo(nome);
        if (arquivo == nullptr) {
            return { false, prefixo_processo(pid) + " não pode deletar o arquivo "
                            + nome + " porque ele não existe." };
        }

        // 3. Usuário comum só pode deletar arquivos criados por ele mesmo.
        //    (Arquivos pré-existentes têm dono = -1, então usuário nunca os apaga.)
        if (!info.tempo_real(pid) && arquivo->dono != pid) {
            return { false, prefixo_processo(pid) + " não pode deletar o arquivo "
                            + nome + " porque não foi criado por ele." };
        }

        // 4. Libera os blocos no disco e tira o arquivo da lista.
        liberar_blocos(*arquivo);
        remover_arquivo(nome);

        return { true, prefixo_processo(pid) + " deletou o arquivo "
                       + nome + "." };
    }

    // Mapa textual da ocupação atual do disco; blocos vazios aparecem como 0.
    string mapa_do_disco() const {
        string mapa = "|";
        for (char bloco : disco) {
            mapa += ' ';
            mapa += (bloco == BLOCO_LIVRE) ? '0' : bloco;
            mapa += " |";
        }
        return mapa;
    }

    int total_blocos()  const { return static_cast<int>(disco.size()); }
    int blocos_livres() const {
        int livres = 0;
        for (char bloco : disco) {
            if (bloco == BLOCO_LIVRE) livres++;
        }
        return livres;
    }

private:
    vector<char>    disco;       // Cada posição guarda a letra do arquivo ou 0.
    vector<Arquivo> arquivos;    // Arquivos atualmente gravados no disco.

    // first-fit: varre o disco do bloco 0 ao fim contando blocos livres
    // consecutivos. Retorna o início da PRIMEIRA sequência livre que couber a
    // quantidade pedida; -1 se nenhuma couber.
    //   - 'inicio'    marca onde começou a sequência livre atual.
    //   - 'contiguos' conta quantos blocos livres seguidos já vimos.
    //   - ao topar com bloco ocupado, zera a contagem e recomeça.
    int encontrar_first_fit(int quantidade) const {
        int inicio = -1;
        int contiguos = 0;

        for (int i = 0; i < static_cast<int>(disco.size()); i++) {
            if (disco[i] == BLOCO_LIVRE) {
                if (contiguos == 0) inicio = i;   // começo de uma nova sequência livre
                contiguos++;
                if (contiguos == quantidade) return inicio;  // achou espaço suficiente
            } else {
                contiguos = 0;   // sequência interrompida por bloco ocupado
                inicio    = -1;
            }
        }
        return -1;   // não há espaço contíguo suficiente
    }

    void ocupar_blocos(int inicio, int quantidade, char nome) {
        for (int i = inicio; i < inicio + quantidade; i++) {
            disco[i] = nome;
        }
    }

    void liberar_blocos(const Arquivo& arquivo) {
        for (int i = arquivo.bloco_inicial;
             i < arquivo.bloco_inicial + arquivo.quantidade_blocos; i++) {
            disco[i] = BLOCO_LIVRE;
        }
    }

    Arquivo* buscar_arquivo(char nome) {
        for (auto& arquivo : arquivos) {
            if (arquivo.nome == nome) return &arquivo;
        }
        return nullptr;
    }

    void remover_arquivo(char nome) {
        for (size_t i = 0; i < arquivos.size(); i++) {
            if (arquivos[i].nome == nome) {
                arquivos.erase(arquivos.begin() + i);
                return;
            }
        }
    }

    static string prefixo_processo(int pid) {
        return "O processo " + to_string(pid);
    }

    // Descreve os blocos ocupados em português: "bloco 0", "blocos 0 e 1",
    // "blocos 0, 1 e 2", etc.
    static string descrever_blocos(int inicio, int quantidade) {
        string texto = (quantidade == 1) ? "bloco " : "blocos ";
        for (int i = 0; i < quantidade; i++) {
            if (i > 0) {
                texto += (i == quantidade - 1) ? " e " : ", ";
            }
            texto += to_string(inicio + i);
        }
        return texto;
    }
};

// =============================================================================
// Leitura e execução do arquivo de entrada do sistema de arquivos (files.txt).
//
// Formato (Seção 2.1.2):
//   Linha 1: quantidade de blocos do disco
//   Linha 2: quantidade de segmentos (arquivos) já ocupados no disco (n)
//   Linhas 3 .. n+2: <arquivo>, <primeiro bloco>, <quantidade de blocos>
//   Linhas n+3 em diante: <ID_Processo>, <Código_Operação>, <Nome_arquivo>
//                         [, <quantidade de blocos>]  (último campo só no criar)
// =============================================================================
namespace LeitorArquivos {

    // Estrutura intermediária com tudo o que foi lido do files.txt.
    struct ConfiguracaoArquivos {
        int                     total_blocos;
        vector<Arquivo>         arquivos_iniciais;
        vector<OperacaoArquivo> operacoes;
    };

    // Remove espaços e tabulações nas extremidades de um campo.
    inline string remover_espacos(const string& texto) {
        size_t inicio = texto.find_first_not_of(" \t\r\n");
        if (inicio == string::npos) return "";
        size_t fim = texto.find_last_not_of(" \t\r\n");
        return texto.substr(inicio, fim - inicio + 1);
    }

    // Separa uma linha por vírgulas, já removendo os espaços de cada campo.
    inline vector<string> separar_campos(const string& linha) {
        vector<string> campos;
        stringstream fluxo(linha);
        string campo;
        while (getline(fluxo, campo, ',')) {
            campos.push_back(remover_espacos(campo));
        }
        return campos;
    }

    // Interpreta o conteúdo do files.txt e devolve a configuração completa.
    //
    // Lê o arquivo em 3 blocos, na ordem definida pela especificação:
    //   [linha 0]      total de blocos do disco
    //   [linha 1]      n = quantos arquivos já estão gravados
    //   [linhas 2..n+1] os n arquivos pré-existentes
    //   [linhas n+2..]  as operações (criar/deletar) a executar
    inline ConfiguracaoArquivos interpretar(const string& conteudo) {
        ConfiguracaoArquivos config;
        stringstream fluxo(conteudo);
        string linha;

        // Coleta apenas as linhas não vazias para simplificar a indexação.
        vector<string> linhas;
        while (getline(fluxo, linha)) {
            if (remover_espacos(linha).empty()) continue;
            linhas.push_back(linha);
        }

        if (linhas.size() < 2) {
            throw invalid_argument("Arquivo de entrada do sistema de arquivos incompleto.");
        }

        // Cabeçalho: tamanho do disco e quantidade de arquivos pré-existentes.
        config.total_blocos = stoi(remover_espacos(linhas[0]));
        int qtd_segmentos    = stoi(remover_espacos(linhas[1]));

        // Bloco 1: os arquivos que já nascem gravados no disco (dono = -1).
        size_t indice = 2;
        for (int i = 0; i < qtd_segmentos; i++, indice++) {
            vector<string> campos = separar_campos(linhas[indice]);
            if (campos.size() != 3) {
                throw invalid_argument("Segmento inicial mal formado: " + linhas[indice]);
            }
            Arquivo arquivo;
            arquivo.nome              = campos[0][0];
            arquivo.bloco_inicial     = stoi(campos[1]);
            arquivo.quantidade_blocos = stoi(campos[2]);
            arquivo.dono              = -1;
            config.arquivos_iniciais.push_back(arquivo);
        }

        // Bloco 2: as operações. Código 0 = criar (tem qtd de blocos no fim);
        // código 1 = deletar (só o nome do arquivo).
        for (; indice < linhas.size(); indice++) {
            vector<string> campos = separar_campos(linhas[indice]);
            if (campos.size() < 3) {
                throw invalid_argument("Operação mal formada: " + linhas[indice]);
            }
            OperacaoArquivo operacao;
            operacao.pid    = stoi(campos[0]);
            operacao.codigo = (stoi(campos[1]) == 0)
                                  ? CodigoOperacao::CRIAR
                                  : CodigoOperacao::DELETAR;
            operacao.nome   = campos[2][0];
            operacao.quantidade_blocos =
                (operacao.codigo == CodigoOperacao::CRIAR && campos.size() >= 4)
                    ? stoi(campos[3])
                    : 0;
            config.operacoes.push_back(operacao);
        }

        return config;
    }

    // Executa todas as operações do files.txt e devolve o texto formatado com
    // o resultado de cada operação e o mapa final de ocupação do disco —
    // exatamente no formato esperado pelo despachante (Seção 2.1.3).
    inline string processar(const string& conteudo, const InfoProcessos& info) {
        ConfiguracaoArquivos config = interpretar(conteudo);

        SistemaDeArquivos sistema(config.total_blocos);
        for (const Arquivo& arquivo : config.arquivos_iniciais) {
            sistema.adicionar_arquivo_inicial(arquivo.nome,
                                              arquivo.bloco_inicial,
                                              arquivo.quantidade_blocos);
        }

        string saida = "Sistema de arquivos =>\n";

        int numero_operacao = 1;
        for (const OperacaoArquivo& operacao : config.operacoes) {
            ResultadoOperacao resultado =
                (operacao.codigo == CodigoOperacao::CRIAR)
                    ? sistema.criar(operacao.pid, info, operacao.nome,
                                    operacao.quantidade_blocos)
                    : sistema.deletar(operacao.pid, info, operacao.nome);

            saida += "\nOperação " + to_string(numero_operacao) + " => "
                     + (resultado.sucesso ? "Sucesso" : "Falha") + "\n";
            saida += resultado.mensagem + "\n";
            numero_operacao++;
        }

        saida += "\nMapa de ocupação do disco:\n";
        saida += sistema.mapa_do_disco() + "\n";

        return saida;
    }

} // namespace LeitorArquivos

#endif // FILESYSTEM_HPP
