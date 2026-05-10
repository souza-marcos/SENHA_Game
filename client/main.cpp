#include <iostream>
#include <string>
#include "controller.cpp"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <host> <porto>" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);

    try {
        Controller client(host, port);
        client.run();
        
    } catch (const NoResponseException& e) {
        std::cerr << "NO RES" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}
