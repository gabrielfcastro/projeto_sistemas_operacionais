GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "Rodando suíte de testes..."

TESTES=("test_despachante" "test_filas" "test_files" "test_memoria" "test_processo" "test_resource")

for teste in "${TESTES[@]}"; do
    ./build/$teste > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo -e "[ ${GREEN}OK${NC} ] $teste"
    else
        echo -e "[ ${RED}FAIL${NC} ] $teste"
    fi
done