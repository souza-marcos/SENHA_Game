#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>
#include <cassert>

#include "../utils/communication.cpp"

class Controller {

    Communication comm;
    int curNumSeg;


    Controller() : comm("0.0.0.0", 8080), curNumSeg(1) {};

    /**
     * Msg do Cliente para o Servidor para iniciar nova comunicação.
     */
    void sendHELLOMessage(){
        Message msg(MessageType::HEL, 0, {});
        comm.sendMessage(msg);
    }

    /**
     * Msg do Cliente para o Servidor para tentar um senha.
     */
    void sendTRYMessage(std::string password){
        std::vector<uint8_t> buffer(8);
        std::memcpy(buffer.data(), password.data(), password.size());
        Message msg(MessageType::TRY, curNumSeg, buffer);
        comm.sendMessage(msg);
        curNumSeg += 1;
    }

    /**
     * Msg do Servidor para o Cliente para indicar a corretude da tentativa válida do cliente.
     */
    void sendREStoTRY(int remAttempts, std::string verification){
        std::vector<uint8_t> buffer(8);
        std::memcpy(buffer.data(), verification.data(), verification.size());
        Message msg(MessageType::RES, remAttempts, buffer);
        comm.sendMessage(msg);
    }

    /**
     * Msg do Cliente para o Servidor para finalizar conexao.
     */
    void sendBYEMessage() {
        Message msg(MessageType::BYE, curNumSeg-1, {});
        comm.sendMessage(msg);
    }

    /**
     * Msg do Servidor para o Cliente para indicar erro na comunicação ou formatação.
     */
    void sendERRMessage(int typeErr){
        Message msg(MessageType::ERR, typeErr, {});
        comm.sendMessage(msg);
    }

    void sendREStoHELLO(int totalAttempts, int sizePassword){
        std::string pass(sizePassword, '?');
        std::vector<uint8_t> buffer(8);
        std::memcpy(buffer.data(), pass.data(), pass.size());
        Message msg(MessageType::RES, totalAttempts, buffer);
        comm.sendMessage(msg);
    }

    void sendREStoBYE(std::string pass){
        std::vector<uint8_t> buffer(8);
        std::memcpy(buffer.data(), pass.data(), pass.size());
        Message msg(MessageType::RES, -1, buffer);
        comm.sendMessage(msg);
    }

    void sendERRMessageInvalidPattern(){
        sendERRMessage(1);
    }

    void sendERRMessageUnexpectedMsg(){
        sendERRMessage(0);
    }

    std::pair<int, std::string> receiveFromTry() {
        Message msg = comm.receiveMessage(12);
        std::string verification(8, ' ');
        std::memcpy((void*) verification.data(), msg.getArr().data(), msg.getArr().size());
        while (verification.back() == ' ') 
            verification.pop_back();
        return {msg.getNumSeq(), verification};
    }
    
    std::pair<int, int> receiveParams(){
        Message msg = comm.receiveMessage(12);
        std::string pass(8, ' ');
        std::memcpy((void*) pass.data(), msg.getArr().data(), msg.getArr().size());
        int sizePass = 0;
        for(char ch : pass) {
            if(ch != '?') break;
            sizePass ++;
        }
        return {msg.getNumSeq(), sizePass};
    }

    std::string receiveFromBYE(){
        auto [numSeq, verification] = receiveFromTry();
        assert(numSeq == -1);
        return verification;
    }
    
    std::string getTryFromUser(){
        return "bla";
    }

    // Visão do cliente
    void runGame(){
        sendHELLOMessage();
        auto[totalAttempts, sizePass] = receiveParams();

        for(int i = 0; i < totalAttempts; i ++) {
            sendTRYMessage(getTryFromUser());
            auto [remainingAttempts, verification] = receiveFromTry();

            std::cout << "Tentativas restantes " << remainingAttempts << " " << verification << std::endl;    
        }
        sendBYEMessage();
        std::cout <<"A senha real era: " << receiveFromBYE() << std::endl;
    }

    // Visão do servidor
    void runGame2(){
        std::string secretPassword = "SENHAA"; 
        int totalAttempts = 5;
        int currentAttempt = 0;

        std::cout << "Servidor aguardando HELLO..." << std::endl;

        Message msg = comm.receiveMessage(4);
        if (msg.getType() == MessageType::HEL) {
            sendREStoHELLO(totalAttempts, secretPassword.size());
        } else {
            sendERRMessageUnexpectedMsg();
            return;
        }

        // 2. Loop de Jogo
        bool gameRunning = true;
        while (gameRunning && currentAttempt < totalAttempts) {
            Message clientMsg = comm.receiveMessage(12);

            if (clientMsg.getType() == MessageType::TRY) {
                currentAttempt++;
                
                // Lógica de verificação (exemplo simples: comparar strings)
                std::vector<uint8_t> clientData = clientMsg.getArr();
                std::string guess(clientData.begin(), clientData.end());
                
                std::string feedback = (guess.find(secretPassword) != std::string::npos) ? "GANHOU" : "ERROU";
                
                sendREStoTRY(totalAttempts - currentAttempt, feedback);
                
                if (feedback == "GANHOU") break;

            } else if (clientMsg.getType() == MessageType::BYE) {
                gameRunning = false;
            }
        }

        // 3. Finalização
        // Se saiu do loop por BYE ou fim de tentativas, envia a resposta final
        sendREStoBYE(secretPassword);
        std::cout << "Partida finalizada." << std::endl;
    }

};

int main() {
    Communication comm("127.0.0.1", 8080);
    Message msg(MessageType::BYE, 1, {1, 2, 3, 4});
    comm.sendMessage(msg);
    return 0;
}