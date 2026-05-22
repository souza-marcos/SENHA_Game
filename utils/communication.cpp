// Iago Zagnoli Albergaria e Marcos Daniel Souza Netto

#include "message.cpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <algorithm>

class CorruptedMessage : std::exception {
    const char* what() const noexcept {
        return "Checksum failed!";
    }
};

class CommunicationFailed : std::exception {
    const char* what() const noexcept {
        return "Unable to communicate!";
    }
};

class TimeoutException : std::exception {
    const char* what() const noexcept {
        return "Timeout!";
    }
};

class NoResponseException : std::exception {
    const char* what() const noexcept {
        return "NO RES!";
    }
};

class Communication {

    int sockfd = -1;
    sockaddr_in ipAddress;

public:
    Communication() = default;

    Communication(int port) {
        this->sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
        if (this->sockfd < 0) throw CommunicationFailed();

        this->ipAddress = {};
        this->ipAddress.sin_port = htons(port);
        this->ipAddress.sin_addr.s_addr = INADDR_ANY;
        this->ipAddress.sin_family = AF_INET;

        if (bind(this->sockfd, (sockaddr*)&this->ipAddress, sizeof(this->ipAddress)) < 0) {
            throw CommunicationFailed();
        }
    }

    Communication(const char* ipAddress, int port) {
        this->sockfd = socket(AF_INET, SOCK_DGRAM, 0); 

        this->ipAddress = {
            .sin_family = AF_INET,
            .sin_port = htons(port)
        };

        if(inet_pton(AF_INET, ipAddress, &this->ipAddress.sin_addr) <= 0) {
            throw CommunicationFailed();
        }
    }

    ~Communication() {
        this->closeConnection();
    }

    void setRecTimeout(int secs) {
        timeval tv;
        tv.tv_sec = secs;
        tv.tv_usec = 0;

        if (setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("failed to set timeout");
        }
    }

    void sendMessage(const Message& msg) {
        std::vector<uint8_t> buffer = msg.serialize();
        
        ssize_t numBytesSent = sendto(
            this->sockfd,
            buffer.data(),
            buffer.size(),
            0,
            (sockaddr*) &this->ipAddress,
            sizeof(this->ipAddress)
        );

        if (numBytesSent < 0) throw CommunicationFailed();
    }

    Message sendWithRetry(const Message& msg, int expectedSize, int maxRetries) {
        for (int i = 0; i <= maxRetries; ++i) {
            try {
                this->sendMessage(msg);
                return this->receiveMessage(expectedSize);
            } catch (const TimeoutException& e) {
                continue;
            } catch (const CorruptedMessage& e) {
                continue;
            }
        }
        throw NoResponseException();
    }
    
    Message receiveMessage(int expectedSize){
        std::vector<uint8_t> buffer(expectedSize);
        socklen_t addrLen = sizeof(this->ipAddress);

        ssize_t numBytesReceived = recvfrom(
            this->sockfd,
            buffer.data(),
            expectedSize, 
            0,
            (sockaddr*) &this->ipAddress,
            &addrLen
        );

        if (numBytesReceived < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
                throw TimeoutException();
            
            throw CommunicationFailed();
        }

        buffer.resize(numBytesReceived);
        
        if (not Message::isValidMessage(buffer)) throw CorruptedMessage();
        
        return Message::desserialize(buffer);
    }

    void closeConnection(){
        if (this->sockfd >= 0) close(this->sockfd), this->sockfd = -1;
    }

    std::string getSenderID() const {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(this->ipAddress.sin_addr), ipStr, INET_ADDRSTRLEN);
        
        int clientPort = ntohs(this->ipAddress.sin_port);
        
        return std::string(ipStr) + ":" + std::to_string(clientPort);
    }

    static std::vector<uint8_t> packText(std::string s) {
        std::vector<uint8_t> buffer(8, 0);
        std::memcpy(buffer.data(), s.data(), std::min((size_t)8, s.size()));
        return buffer;
    }

    static std::string unpackText(std::vector<uint8_t> buffer){
        std::string s(buffer.begin(), buffer.end());
        while (not s.empty() and (s.back() == '\0' or s.back() == ' '))
            s.pop_back();
        return s;
    }
};
