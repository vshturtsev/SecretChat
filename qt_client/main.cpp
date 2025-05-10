#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>
#include <string>

#include "LoginService.hpp"
#include "LoginView.hpp"
#include "message_data.hpp"
#include "qt_client.hpp"

// QPushButton *btn = nullptr;

// void onClick() {
//     static int count = 0;
//     btn->setText(QString::number(++count));

// }

int main(int argc, char *argv[]) {
  Client cli;
  int fd = cli.run();
  // int fd =1000; //TEST
  if (fd < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  QApplication app(argc, argv);

  LoginService l_service(fd);
  LoginView login_window(&l_service);

  login_window.show();

  return app.exec();
  // QWidget* widget = new QWidget;

  // widget->setWindowTitle("SecretChat App");
  // widget->setMinimumHeight(600);
  // widget->setMinimumWidth(400);

  //    QLabel label{widget};
  //    label.setText("Hello!");

  // btn = new QPushButton(QIcon("./images/send.png"),"Send");
  // QObject::connect(btn, &QPushButton::clicked, onClick);

  //     QVBoxLayout *main_layout = new QVBoxLayout(widget);

  //     QTextEdit *chat_history = new QTextEdit();
  //     chat_history->setReadOnly(true);
  //     chat_history->setTextInteractionFlags(Qt::TextSelectableByMouse);
  //     chat_history->verticalScrollBar()->setValue(chat_history->verticalScrollBar()->maximum());

  //     main_layout->addWidget(chat_history);

  //     QHBoxLayout *input_layout = new QHBoxLayout();

  //     QTextEdit *message_input = new QTextEdit();
  //     message_input->setMaximumHeight(80);
  //     QPushButton *send_button = new QPushButton("Send");

  //     input_layout->addWidget(message_input);
  //     input_layout->addWidget(send_button);

  //     main_layout->addLayout(input_layout);

  //     main_layout->setStretch(0,1);
  //     main_layout->setStretch(1,0);
  //     main_layout->setContentsMargins(5,5,5,5);
  //     input_layout->setSpacing(5);
  //     // std::string test("asad");
  //     // cli.send_chat(test);
  //     QObject::connect(send_button, &QPushButton::clicked, [&cli,
  //     message_input, chat_history]() {
  //         QString message = message_input->toPlainText();
  //         if(!message.isEmpty()) {
  //             chat_history->append("Вы: " + message);
  //             std::string str = message.toStdString();
  //             cli.send_chat(str);
  //             message_input->clear();
  //         }
  //     });

  // //    layout->setDirection(QBoxLayout::Direction::BottomToTop);
  //     // std::cout << main_layout->count() << std::endl;
  // //    label.setText(std::to_string( layout->count()).c_str());
}