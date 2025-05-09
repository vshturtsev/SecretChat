#pragma once
#include "../common/message_data.hpp"

#include <QObject>

class LoginService : public QObject {
  Q_OBJECT

public:
  explicit LoginService(int _fd, QObject *parent = nullptr);
  ~LoginService();

  int authentication(std::string& username, std::string& user_password, MsgType type); 
  int send_message(std::string& message); 

signals:
  void loginSuccess();
  void loginFailed(const QString &error);

private:
  int fd;

};