#include "message_data.hpp"


#include <arpa/inet.h>
// #include <cstring>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

std::vector<uint8_t> marshaling(MsgHeader &headers) {
  uint32_t net_type = htonl(static_cast<uint32_t>(headers.type));
  uint32_t net_len = htonl(headers.len);
  std::vector<uint8_t> bytes;
  bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&headers.type),
               reinterpret_cast<uint8_t *>(&headers.type) +
                   sizeof(headers.type));

  bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&headers.len),
               reinterpret_cast<uint8_t *>(&headers.len) + sizeof(headers.len));
  return bytes;
}

MsgHeader demarshaling_header(std::vector<uint8_t> &bytes) {
  MsgHeader headers = {};
  memcpy(&headers.type, bytes.data(), sizeof(headers.type));
  memcpy(&headers.len, bytes.data() + sizeof(headers.type),
         sizeof(headers.len));
  return headers;
}

std::string demarshaling_string(std::vector<uint8_t> &bytes) {
  return std::string(bytes.begin(), bytes.end());
}

std::ostringstream ChatMessage::get_display_view()  {
  std::ostringstream oss;
  oss << sender_name << " "
      << std::put_time(std::localtime(&msg_time), "%Y-%m-%d %H:%M:%S:\n")
      << content; // << std::endl;
  return oss;
}

// std::string encode(const std::string &str) {}
