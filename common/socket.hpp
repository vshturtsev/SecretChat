#pragma once

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
  byte recv_all(const int fd);
  int send_all(const int fd, const byte& bytes);
  int turn_on_nonblock(int fd);
}