#ifndef FORM_LOGIN_H
#define FORM_LOGIN_H

#include <QWidget>

#include "tools.h"

namespace Ui {
class FormLogin;
}

class FormLogin : public QWidget
{
    Q_OBJECT

public:
    explicit FormLogin(QWidget *parent = nullptr);
    ~FormLogin();

    void clearData();

    QString getUsername();
    QString getPassword();

private:
    Ui::FormLogin *ui;

    QString username;
    QString password;

private:
    void InitMembers();
    void InitConnections();

private slots:
    void submitForm();

signals:
    void completed();

};

#endif // FORM_LOGIN_H
