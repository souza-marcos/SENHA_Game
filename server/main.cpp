#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <iostream>
#include <cstring>

int main() {
    // AF_INET -> ipv4, SOCK_DGRAM -> UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddress{
        .sin_family = AF_INET,        // ipv4
        .sin_port = htons(8080),      // 8080
        .sin_addr {
            .s_addr = INADDR_ANY // 0.0.0.0
        }
    };

    // Binding
    bind(sockfd, (sockaddr *)&serverAddress, sizeof(serverAddress));

    char buffer[1024];
    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);

    std::cout << "Waiting for data ..." << std::endl;

    int n = recvfrom(sockfd, buffer, 1024, 0, (sockaddr *)&clientAddress, &clientLen);
    buffer[n] = '\0';

    std::cout << "Received: " << buffer << std::endl;
    close(sockfd);

    return 0;
}


