// main.cpp — Ponto de entrada do pseudo-SO
//
// Uso: ./dispatcher processes.txt files.txt string.txt
//
// processes.txt — um processo por linha (Seção 2.1.2)
// files.txt     — configuração do disco + operações de arquivo (Seção 1.4)
// string.txt    — string de referência de páginas por processo (Seção 1.2)

#include <iostream>
#include "despachante.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0]
             << " <processes.txt> <files.txt> <string.txt>\n";
        return 1;
    }

    string arquivo_processos = argv[1];
    string arquivo_files     = argv[2];
    string arquivo_strings   = argv[3];

    try {
        Despachante despachante;
        despachante.carregar_arquivos(arquivo_processos, arquivo_strings, arquivo_files);
        despachante.executar();
    } catch (const exception& e) {
        cerr << "Erro fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}