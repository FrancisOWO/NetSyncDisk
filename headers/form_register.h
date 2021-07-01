#ifndef FORM_REGISTER_H
#define FORM_REGISTER_H

#include <QWidget>
#include <QString>

#include "tools.h"

namespace Ui {
class FormRegister;
}

class FormRegister : public QWidget
{
    Q_OBJECT

public:
    explicit FormRegister(QWidget *parent = nullptr);
    ~FormRegister();

    void clearData();

    QString getUsername();
    QString getPassword();

private:
    Ui::FormRegister *ui;

    QString username;
    QString password;

private:
    void InitMembers();
    void InitConnections();

    int checkUname();
    int checkPwd();

private slots:
    void submitForm();

    void changePwd1Vis();
    void changePwd2Vis();

signals:
    void completed();

};

#endif // FORM_REGISTER_H
