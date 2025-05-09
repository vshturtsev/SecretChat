#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "LoginService.hpp"
#include "LoginView.hpp"
#include "../common/message_data.hpp"

LoginView::LoginView(LoginService *service, QWidget *parent)
    : QDialog(parent), l_service(service) {
  resize(QSize(200, 100));
  login_edit = new QLineEdit();
  password_edit = new QLineEdit();
  remember_check = new QCheckBox("Запомнить данные");
  login_button = new QPushButton("Вход");
  register_button = new QPushButton("Регистрация");

  // compose window
  QVBoxLayout *main_layout = new QVBoxLayout(this);

  // add login/password fields and checkbox "Remember"
  QFormLayout *logpass_layout = new QFormLayout();
  logpass_layout->addRow("Логин:", login_edit);
  logpass_layout->addRow("Пароль:", password_edit);
  main_layout->addLayout(logpass_layout);
  // Qt::AlignmentFlag align();
  main_layout->addWidget(remember_check, 0, Qt::AlignHCenter);

  // add buttons
  QHBoxLayout *buttons = new QHBoxLayout();
  buttons->addWidget(login_button);
  buttons->addWidget(register_button);
  main_layout->addLayout(buttons);

  connect(login_button, SIGNAL(clicked()), this, SLOT(on_login_button_clicked()));
  connect(register_button, SIGNAL(clicked()), this, SLOT(on_register_button_clicked()));
}

LoginView::~LoginView() {}

void LoginView::on_login_button_clicked() {

}
void LoginView::on_register_button_clicked() {
  std::string username = login_edit->text().toStdString();
  std::string password = password_edit->text().toStdString();
  l_service->authentication(username, password, MsgType::Reg);

}