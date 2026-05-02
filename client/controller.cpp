#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>
#include <cassert>

enum class MessageType : uint8_t {
    HEL = 1,
    TRY = 2,
    RES = 3,
    BYE = 4,
    ERR = 5
};

class Message {
    MessageType type;      
    int16_t numSeq;
    std::vector<uint8_t> arr;
    // CheckSum so eh calculado no envio e no recebimento

    size_t getSizeMessage() const {
        return (this->type == MessageType::TRY || this->type == MessageType::RES) ? 12 : 4;
    }

public:
    Message() = default;

    Message(MessageType type, int16_t numSeq, std::vector<uint8_t> arr) {
        this->type = type;
        this->numSeq = numSeq;
        this->arr = arr;
    }

    MessageType getType() const {
        return this->type;
    }

    int16_t getNumSeq() const {
        return this->numSeq;
    }

    std::vector<uint8_t> getArr() const {
        return this->arr;
    }

    std::vector<uint8_t> serialize() const {
        size_t size_message = getSizeMessage();
        std::vector<uint8_t> buffer(size_message);

        buffer[0] = (uint8_t) type;
    
        int16_t numSeqToNet = htons(numSeq);
        std::memcpy(&buffer[2], &numSeqToNet, 2);

        if (size_message == 12 and arr.size() >= 8){
            std::memcpy(&buffer[4], arr.data(), 8);
        } 
    
        uint8_t checkSum = 0;
        for (uint8_t byte : buffer) {
            checkSum ^= byte;
        }

        buffer[1] = checkSum;
        return buffer;
    }

    static Message desserialize(std::vector<uint8_t> &buffer){
        Message msg;
        msg.type = (MessageType) buffer[0];

        int16_t numSeqFromNet;
        std::memcpy(&numSeqFromNet, &buffer[2], 2);
        msg.numSeq = ntohs(numSeqFromNet);

        if (buffer.size() == 12){
            msg.arr.resize(8);
            std::memcpy(msg.arr.data(), &buffer[4], 8);
        }
        return msg;
    }

    static bool isValidMessage(std::vector<uint8_t>& buffer){
        uint8_t val = 0;
        for(uint8_t byte : buffer) val ^= byte;
        return val == 0;
    }
};

class Communication {

    int sockfd = -1;
    sockaddr_in serverAddress;

public:
    Communication() = default;

    Communication(const char* serverAddress, int port) {
        this->sockfd = socket(AF_INET, SOCK_DGRAM, 0); 

        this->serverAddress = {
            .sin_family = AF_INET,
            .sin_port = htons(port)
        };

        if(inet_pton(AF_INET, serverAddress, &this->serverAddress.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
        }
    }

    ~Communication() {
        this->closeConnection();
    }

    void sendMessage(const Message& msg) {
        std::vector<uint8_t> buffer = msg.serialize();
        if(buffer.size() == 0){
            std::cerr << "Empty message" << std::endl;
            return;
        }

        size_t numBytesSent = sendto(
            this->sockfd,
            buffer.data(),
            buffer.size(),
            0,
            (sockaddr*) &this->serverAddress,
            sizeof(this->serverAddress)
        );

        if (numBytesSent < 0) std::cerr << "Failed to send the message" << std::endl;
        else std::cout << "Sent " << numBytesSent << " bytes" << std::endl;  
    }
    
    Message receiveMessage(int expectedSize){
        std::vector<uint8_t> buffer(expectedSize);
        socklen_t addrLen = sizeof(this->serverAddress);

        size_t numBytesReceived = recvfrom(
            this->sockfd,
            buffer.data(),
            expectedSize, 
            0,
            (sockaddr*) &this->serverAddress,
            &addrLen
        );

        if (numBytesReceived < 0) std::cerr << "Failed to receive the message" << std::endl;
        else std::cout << "Receive " << numBytesReceived << " bytes" << std::endl;
    
        if(not Message::isValidMessage(buffer)) {
            std::cerr << "Message corrupted" << std::endl;
        }
        return Message::desserialize(buffer);
    }

    void closeConnection(){
        if (this->sockfd >= 0) close(this->sockfd), this->sockfd = -1;
    }
};

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
        std::memcpy(verification.data(), msg.getArr().data(), msg.getArr().size());
        while (verification.back() == ' ') 
            verification.pop_back();
        return {msg.getNumSeq(), verification};
    }
    
    std::pair<int, int> receiveParams(){
        Message msg = comm.receiveMessage(12);
        std::string pass(8, ' ');
        std::memcpy(pass.data(), msg.getArr().data(), msg.getArr().size());
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