#pragma once
#include "LoginService.hpp"

#include <QCheckBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class LoginService;

class LoginView : public QDialog {
  Q_OBJECT

public:
  explicit LoginView(LoginService *service, QWidget *parent = nullptr);
  ~LoginView();

private slots:
  void on_login_button_clicked();
  void on_register_button_clicked();

private:
  LoginService *l_service;
  QLineEdit *login_edit;
  QLineEdit *password_edit;
  QCheckBox *remember_check;
  QPushButton *login_button;
  QPushButton *register_button;
};