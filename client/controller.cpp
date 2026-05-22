// Iago Zagnoli Albergaria e Marcos Daniel Souza Netto

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include "../utils/communication.cpp"

class Controller {
    Communication comm;

    int curNumSeq;
    const int timeoutSecs = 1;
    const int maxRetries = 2;

    void noRes() {
        std::cout << "NO RES" << std::endl;
        exit(1);
    }

public:
    Controller(std::string host, int port) : comm(host.c_str(), port), curNumSeq(1) {
        comm.setRecTimeout(timeoutSecs);
    }

    void run() {
        Message hello(MessageType::HEL, 0, {});
        Message resHello = comm.sendWithRetry(hello, 12, this->maxRetries);
        
        if (resHello.getType() == MessageType::RES) {
            int numberAttempts = resHello.getNumSeq();
            std::string pattern = Communication::unpackText(resHello.getArr());
            int sizePass = 0;
            for (char c : pattern) if (c == '?') sizePass++;
            
            std::cout << "NA=" << sizePass << " NT=" << numberAttempts << std::endl;
        } 
        else if (resHello.getType() == MessageType::ERR) {
            
            if (resHello.getNumSeq() > 0) std::cout << "RETRY " << resHello.getNumSeq() << std::endl;
            else { 
                std::cout << "ERRO" << std::endl;
                exit(0); 
            }
        }

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            Message tryMsg(MessageType::TRY, curNumSeq, Communication::packText(line));
            Message res = comm.sendWithRetry(tryMsg, 12, this->maxRetries);

            if (res.getType() == MessageType::RES) {
                std::string verification = Communication::unpackText(res.getArr());
                std::cout << curNumSeq << "(" << res.getNumSeq() << ") " << verification << std::endl;
                this->curNumSeq++;
           
            } else if (res.getType() == MessageType::ERR) {
                if (res.getNumSeq() > 0) {
                    std::cout << "RETRY " << res.getNumSeq() << std::endl;
                    
                } else {
                    std::cout << "ERRO" << std::endl;
                    exit(0);
                }
            }
        }

        Message bye(MessageType::BYE, curNumSeq - 1, {});
        Message resBye = comm.sendWithRetry(bye, 12, this->maxRetries);
        if (resBye.getType() == MessageType::RES) {
            std::string pass = Communication::unpackText(resBye.getArr());
            std::cout << "Senha=" << pass.c_str() << std::endl;
        }
    }
};
