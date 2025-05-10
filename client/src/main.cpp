#include <system_error>
#include <optional>
#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>

#include "../../common/message_data.hpp"
#include "../../common/socket.hpp"

// using json = nlohmann::json;
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
    int authentication(std::string& username, std::string& password, int fd, ReqType type) {

        AuthMessage auth_message(username, password);
        std::string json_data = MessageService::to_string(std::move(auth_message));
        Request request(type, std::move(json_data));
        bytes data = request.marshaling();
        std::cout << "message: " << json_data.size() << "\n";
        int status = SocketService::send_all(fd, data);
        return status;
    }
    int send_chat(int fd, std::string& message) {
        Message chat_message (ReqType::Chat, message);
        std::string json_data =  MessageService::to_string(std::move(chat_message));
        Request request(ReqType::Chat, std::move(json_data));
        bytes data = request.marshaling();
        int status = SocketService::send_all(fd, data);
        return status;
    }
    void read_broadcast(int fd) {
        std::string buffer;

        while (1) {
            buffer.clear();
            buffer.resize(1024);
            std::cout << "ready broadcast\n";
            ssize_t receive = recv(fd, buffer.data(), buffer.size(), 0);
            if (receive == 0) {
                connected = false;
                return;
            }
            if (receive < 0) {
                connected = false;
                perror("failed read to server");
                return;
            }
            ChatMessage chat_msg = MessageService::from_string<ChatMessage>(buffer);
            std::cout << chat_msg.get_display_view().str() << std::endl;
        }
    }
public:
    void run()
    {
        int write_fd = connect();
        if (write_fd < 0) return;
        connected = true;
        // Get from Vovan QT type of message
        
        std::string username;
        std::string password;
        std::getline(std::cin, username);
        std::getline(std::cin, password);

        if (authentication(username, password, write_fd, ReqType::Reg) < 0) return;
        int read_fd = dup(write_fd);
        std::thread t([&,this]{ read_broadcast(read_fd); });
        std::string msg;
        while (std::getline(std::cin, msg)) {
            if (send_chat(write_fd, msg) <= 0) {
                std::cout << "close connection" << "\n";
                break;
            }
        }
        close(write_fd);
        close(read_fd);
        t.join();
    }
};


int main(int argc, char const *argv[])
{
    Client cli;
    cli.run();
    return 0;
}
