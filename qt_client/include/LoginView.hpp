#pragma once

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QPushButton>

#include "LoginService.hpp"

class LoginService; 

class LoginView : public QDialog {
    // Q_OBJECT

    public:
    explicit LoginView(LoginService *service, QWidget *parent = nullptr);
    ~LoginView(); 

    private slots:
    void handleLogin();
    void handleRegister();
    
    private:
    LoginService *l_service;
    QLineEdit *login_edit;
    QLineEdit *password_edit;
    QCheckBox *remember_check;
    QPushButton *login_button;
    QPushButton *register_button;
};