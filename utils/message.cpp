#include <arpa/inet.h> 
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

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