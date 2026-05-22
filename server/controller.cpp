// Iago Zagnoli Albergaria e Marcos Daniel Souza Netto

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <map>
#include "../utils/communication.cpp"

#define sz(x) (int) x.size()

struct ClientState {
    int lastClientSeq = 0;
    int curNumberAttempt = 0;
    Message lastRes;
    bool active = true;
};

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
        std::map<std::string, ClientState> activeClients;
        int finishedClients = 0;

        while (finishedClients < 2) {
            // try {
                Message msg = comm.receiveMessage(12);
                std::string senderID = comm.getSenderID();
                
                if(!activeClients.count(senderID)){
                    if (msg.getType() != MessageType::HEL)
                        continue;

                    activeClients[senderID] = ClientState();
                }

                ClientState& curState = activeClients[senderID];
                
                if (!curState.active) {
                    if (msg.getType() == MessageType::BYE) {
                        comm.sendMessage(curState.lastRes);
                    }
                    continue;
                }

                std::string guess; 

                switch (msg.getType()){
                    case MessageType::HEL:
                        curState.lastRes = Message(MessageType::RES, numberAttempts, Communication::packText(std::string(sizePass, '?')));
                        comm.sendMessage(curState.lastRes);
                        break;

                    case MessageType::TRY: 
                        if (msg.getNumSeq() <= curState.lastClientSeq) {    
                            comm.sendMessage(curState.lastRes);
                            continue;
                        }

                        if (msg.getNumSeq() != curState.lastClientSeq + 1 || curState.curNumberAttempt >= numberAttempts) {
                            comm.sendMessage(Message(MessageType::ERR, 0, {}));
                            continue;
                        }
                        
                        guess = Communication::unpackText(msg.getArr());
                        if (!isValidPassword(guess) or sz(guess)!= sizePass) {
                            curState.lastRes = Message(MessageType::ERR, msg.getNumSeq(), {});
                            comm.sendMessage(curState.lastRes);
                        } else {
                            curState.lastClientSeq = msg.getNumSeq();
                            curState.curNumberAttempt++;
                            std::string feedBack = getFeedback(guess);
                            curState.lastRes = Message(MessageType::RES, numberAttempts - curState.curNumberAttempt, Communication::packText(feedBack));
                            comm.sendMessage(curState.lastRes);
                        }
                        break;

                    case MessageType::BYE:
                        curState.lastRes = Message(MessageType::RES, -1, Communication::packText(pass));
                        comm.sendMessage(curState.lastRes);
                        curState.active = false; 
                        finishedClients++;
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
