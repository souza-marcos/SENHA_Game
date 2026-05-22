// Iago Zagnoli Albergaria e Marcos Daniel Souza Netto

#include <iostream>
#include <string>
#include <vector>
#include "controller.cpp"

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cerr << "Uso: " << argv[0] << " <porto> <senha> <tentativas>" << std::endl;
            exit(1);
        }
        
        int port = std::stoi(argv[1]);
        std::string password = argv[2];
        int numberAttempts = std::stoi(argv[3]);

        Controller server(port, password, numberAttempts);
        server.run();
    } catch (std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }
    return 0;
}