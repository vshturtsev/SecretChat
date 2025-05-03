#include <sys/socket.h>
#include <string_view>
#include <system_error>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <optional>
#include <iostream>
#include <nlohmann/json.hpp>
#include <cstring>
#include <thread>
#include <atomic>

// #include <sys/time.h>
#include "../../common/message_data.hpp"

using json = nlohmann::json;
using std::cout;
#define USER_ID 123456  // get by cl

struct Config {
    std::string_view ip;
    uint16_t port;
};

bool translate_addres(const Config& config, sockaddr_in& address) {       
    inet_pton(AF_INET, config.ip.data(), &address.sin_addr);
    address.sin_port = htons(config.port);
    return true;
}
        

class Client {
    Config config {
        // .ip = "217.171.146.254",
        .ip = "127.0.0.1",
        .port = 9090
    };
    
    sockaddr_in addres;
    std::atomic<bool> connected;
    int create_socket() {
        if (!translate_addres(config, addres)) {
            perror("failed translate address to networ format");
            return -1;
        }
        addres.sin_family = AF_INET;
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("failed create socket");
        }
        return fd;
    }
    int connect() {
        int fd = create_socket();
        if (fd < 0)
            return -1;
        int result = ::connect(fd, reinterpret_cast<sockaddr*>(&addres), sizeof(addres));
        if (result) {
            perror("failed connect server");
            return -1;
        }
        return fd;
    }
    int authentication(int fd) {
        std::string login = "vova";
        std::string password = "true";
        Message message (MsgType::Auth, login + ":" + password);
        
        std::string json_message = MessageService::to_string(message);
        
        const MsgHeader headers = {
            .type = message.get_type(),
            .len = json_message.size()
        };

        cout << headers.len << " " << sizeof(headers) << "\n";
        ssize_t sent = send(fd, &headers, sizeof(headers), 0);
        if (sent != sizeof(headers)) {
            perror("failed send headers");
        }
        int status = send_message(fd, json_message);
        return status;
    }
    int send_chat(int fd, std::string& message) {
        Message message_chat (MsgType::Chat, message);
        std::string json_message =  MessageService::to_string(message_chat);
        
        const MsgHeader headers = {
            .type = message_chat.get_type(),
            .len = json_message.size()
        };

        cout << headers.len << " " << sizeof(headers) << "\n";
        ssize_t sent = send(fd, &headers, sizeof(headers), 0);
        if (sent != sizeof(headers)) {
            perror("failed send headers");
        }
        int status = send_message(fd, json_message);
        return status;
    }
    int send_message(int fd, std::string& message) {
        if (!connected) return 0;
        ssize_t sent = send(fd, message.data(), message.size(), MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EPIPE) {
                std::cout << "server disconnect: send" << '\n';
                return 0;
            }
            perror("faild send to server");
            return -1;
        }
        return 1;
    }
    void read_broadcast(int fd) {
        std::string buffer;
        buffer.resize(1024);
        while (1) {
            ssize_t receive = recv(fd, buffer.data(), buffer.size(), 0);
            if (receive == 0) {
                connected = false;
                return;
            }
            if (receive < 0) {
                perror("failed read to server");
                return;
            }
            // std::cout << buffer << std::endl;
            ChatMessage chat_msg = MessageService::from_string<ChatMessage>(buffer);
            std::cout << chat_msg.get_display_view().str() << std::endl;
            buffer.clear();
        }
    }
public:
    void run()
    {
        int fd = connect();
        if (fd < 0) return;
        connected = true;
        if (!authentication(fd)) return;
        std::thread t([&,this]{ read_broadcast(fd); });
        std::string msg;
        while (std::getline(std::cin, msg)) {
            if (send_chat(fd, msg) <= 0) {
                std::cout << "close connection" << "\n";
                break;
            }
        }
        t.join();
    }
};


int main(int argc, char const *argv[])
{
    Client cli;
    cli.run();
    return 0;
}
