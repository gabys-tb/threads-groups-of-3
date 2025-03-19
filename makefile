# Nome do executável
EXEC = meu_programa

# Compilador e flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

# Alvo padrão é 'build'
default: build

# Regra para compilar e gerar o executável
build: $(EXEC)

$(EXEC): main.cpp
	$(CXX) $(CXXFLAGS) -o $(EXEC) main.cpp

# Regra para executar o programa
run: build
	./$(EXEC)

# Regra para limpar os arquivos compilados
clean:
	rm -f $(EXEC) *~
