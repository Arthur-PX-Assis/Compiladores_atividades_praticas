#include "csmall.h"

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Uso: %s arquivo.csmall [saida.tac]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *nomeSaida = (argc == 3) ? argv[2] : "programa_tac.tac";
    CodigoFonte codigo;
    if (!abrirCodigoFonte(&codigo, argv[1])) return EXIT_FAILURE;

    FILE *saida = fopen(nomeSaida, "w");
    if (!saida) {
        fprintf(stderr, "Não foi possível criar o arquivo de saída: %s\n", nomeSaida);
        fecharCodigoFonte(&codigo);
        return EXIT_FAILURE;
    }

    Parser parser = {0};
    parser.codigo = &codigo;
    parser.saidaTac = saida;
    int sucesso = analisarPrograma(&parser);

    fclose(saida);
    fecharCodigoFonte(&codigo);

    if (sucesso) {
        printf("Compilação concluída com sucesso. TAC salvo em '%s'.\n", nomeSaida);
        return EXIT_SUCCESS;
    }

    remove(nomeSaida);
    fprintf(stderr, "A compilação falhou. O arquivo TAC não foi mantido.\n");
    return EXIT_FAILURE;
}
