#include <vector>
#include <cstdint>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>


using byte = std::vector<uint8_t>;

namespace SocketService {

int send_all(const int fd, const byte& bytes) {
    size_t total_sent = 0;
    ssize_t current_sent = 0;
    while (total_sent < bytes.size()) {
        current_sent = ::send(fd, bytes.data() + total_sent, bytes.size() - total_sent, MSG_NOSIGNAL);
        if (current_sent < 0) {
            if (errno == EPIPE) {
                std::cout << "server disconnect: send" << '\n';
                return 0;
            }
            perror("faild send to server");
            return -1;
        }
        total_sent += current_sent;
    }
    std::cout << "From client size message: " << total_sent << std::endl;
    return total_sent;
}

byte recv_all(const int fd) {
    byte data;
    byte  buffer;
    buffer.resize(2);
    ssize_t recived_bytes{};
    while ((recived_bytes = ::recv(fd, buffer.data(), buffer.size(), 0)) > 0) {
      data.insert(data.end(), buffer.begin(), buffer.begin() + recived_bytes);
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK || recived_bytes == 0) {
      std::cout << "not data\n";  
    } else {
      std::cerr << "error recv\n";
      data.clear();
    }
    return data;
}

int turn_on_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags > 0)
    flags |= O_NONBLOCK;
  
  if (flags > 0)
    flags = fcntl(fd, F_SETFL, flags);
  
  return flags;
}
}