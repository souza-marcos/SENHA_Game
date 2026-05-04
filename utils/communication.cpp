#include "message.cpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

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
