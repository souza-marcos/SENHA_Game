#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <map>
#include "../utils/communication.cpp"

#define sz(x) (int) x.size()

class Controller {
    Communication comm;
    std::string pass;
    int numberAttempts;
    int sizePass;

    bool isValidPassword(const std::string& p) {
        if (p.size() < 4 or p.size() > 8) return false;
        std::vector<bool> seen(10, false);
        for (char c : p) {
            if (c < '0' or c > '9') return false;
            if (seen[c - '0']) return false;
            seen[c - '0'] = true;
        }
        return true;
    }

    std::string generateRandomPassword(int size) {
        std::vector<int> digits = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::mt19937 rng(time(nullptr));
        std::shuffle(digits.begin(), digits.end(), rng);
        std::string res = "";
        for (int i = 0; i < size; ++i) 
            res.append(std::to_string(digits[i]));
        return res;
    }

    std::string getFeedback(const std::string& guess) {
        std::string res(sizePass, '-');
        for (int i = 0; i < std::min(sizePass, sz(guess)); i ++) {
            if (guess[i] == pass[i]) 
                res[i] = '*';
            else if (pass.find(guess[i]) != std::string::npos)
                res[i] = '+';
        }
        return res;
    }

public:
    Controller(int port, std::string pass, int nt) : comm(port), numberAttempts(nt) {
        bool allZeros = true;
        for(char c : pass) {
            if(c != '0') allZeros = false; 
        };
        this->sizePass = pass.size();

        if (allZeros) this->pass = generateRandomPassword(sizePass);
        else {
            if (!isValidPassword(pass)) exit(1);
            this->pass = pass;
        }
    }

    void run() {
        int lastClientSeq = 0;
        int curNumberAttempt = 0;
        Message lastRes;
        bool clientActive = true;

        while (clientActive) {
            // try {
                Message msg = comm.receiveMessage(12);
                std::string guess;

                switch (msg.getType()){
                    case MessageType::HEL:
                        lastRes = Message(MessageType::RES, numberAttempts, Communication::packText(std::string(sizePass, '?')));
                        comm.sendMessage(lastRes);

                        break;

                    case MessageType::TRY: 
                        if (msg.getNumSeq() <= lastClientSeq) {    
                            comm.sendMessage(lastRes);
                            continue;
                        }

                        if (msg.getNumSeq() != lastClientSeq + 1 || curNumberAttempt >= numberAttempts) {
                            comm.sendMessage(Message(MessageType::ERR, 0, {}));
                            continue;
                        }
                        
                        guess = Communication::unpackText(msg.getArr());
                        if (!isValidPassword(guess) or sz(guess)!= sizePass) {
                            lastRes = Message(MessageType::ERR, msg.getNumSeq(), {});
                            comm.sendMessage(lastRes);
                        } else {
                            lastClientSeq = msg.getNumSeq();
                            curNumberAttempt++;
                            std::string feedBack = getFeedback(guess);
                            lastRes = Message(MessageType::RES, numberAttempts - curNumberAttempt, Communication::packText(feedBack));
                            comm.sendMessage(lastRes);
                        }
                        break;

                    case MessageType::BYE:
                        lastRes = Message(MessageType::RES, -1, Communication::packText(pass));
                        comm.sendMessage(lastRes);
                        clientActive = false;
                        break;

                    default:
                        comm.sendMessage(Message(MessageType::ERR, 0, {}));
                        break;
                }
            // } catch (const std::exception& e) {
            //     std::cout << e.what() << std::endl;
            //     continue;
            // }
        }
    }
};
