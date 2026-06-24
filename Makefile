CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -pthread -I src

SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

TARGET = dispatcher

TEST_PROCESSO       = $(BUILD_DIR)/test_processo
TEST_FILAS          = $(BUILD_DIR)/test_filas
TEST_DESPACHANTE    = $(BUILD_DIR)/test_despachante

TEST_MEMORY         = $(BUILD_DIR)/test_memory
TEST_RESOURCE       = $(BUILD_DIR)/test_resource
TEST_FILES          = $(BUILD_DIR)/test_files

.PHONY: all test test_processo test_filas test_despachante test_memoria test_resource test_arquivos clean

all: $(TARGET)

$(TARGET): | $(BUILD_DIR)
	@echo "Aviso: O programa principal (dispatcher) ainda nao foi implementado."

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_PROCESSO): $(TEST_DIR)/test_processo.cpp $(SRC_DIR)/processo.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(TEST_FILAS): $(TEST_DIR)/test_filas.cpp $(SRC_DIR)/filas.hpp $(SRC_DIR)/processo.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(TEST_DESPACHANTE): $(TEST_DIR)/test_despachante.cpp $(SRC_DIR)/despachante.hpp $(SRC_DIR)/filas.hpp $(SRC_DIR)/processo.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(TEST_MEMORY): $(TEST_DIR)/test_memory.cpp $(SRC_DIR)/memory.hpp $(SRC_DIR)/processo.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(TEST_RESOURCE): $(TEST_DIR)/test_resource.cpp $(SRC_DIR)/resource.hpp $(SRC_DIR)/processo.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(TEST_FILES): $(TEST_DIR)/test_files.cpp $(SRC_DIR)/filesystem.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

test_processo: $(TEST_PROCESSO)
	@echo "\n>>> Rodando testes: Módulo de processos"
	@./$(TEST_PROCESSO)

test_filas: $(TEST_FILAS)
	@echo "\n>>> Rodando testes: Módulo de filas"
	@./$(TEST_FILAS)

test_despachante: $(TEST_DESPACHANTE)
	@echo "\n>>> Rodando testes: Módulo do Despachante"
	@./$(TEST_DESPACHANTE)

test_memoria: $(TEST_MEMORY)
	@echo "\n>>> Rodando testes: Módulo de Memória"
	@./$(TEST_MEMORY)

test_resource: $(TEST_RESOURCE)
	@echo "\n>>> Rodando testes: Módulo de Recursos"
	@./$(TEST_RESOURCE)

test_arquivos: $(TEST_FILES)
	@echo "\n>>> Rodando testes: Módulo de Arquivos"
	@./$(TEST_FILES)

test: test_processo test_filas test_despachante
	@echo "\nTodos os testes concluídos."

clean:
	rm -rf $(BUILD_DIR) $(TARGET)