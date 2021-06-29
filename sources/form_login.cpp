#include "form_login.h"
#include "ui_form_login.h"
#include <QMessageBox>
#include <QDebug>

FormLogin::FormLogin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormLogin)
{
    ui->setupUi(this);

    InitMembers();
    InitConnections();
}

FormLogin::~FormLogin()
{
    delete ui;
}

void FormLogin::InitMembers()
{
    //Ĭ�ϵ��û��������룬�������
    ui->lnUsername->setText("test_user_1");
    ui->lnPwd->setText("123abcde_");

    //�������볤��
    const int uname_max_len = ULimits::uname_max_len;
    const int pwd_max_len = ULimits::pwd_max_len;
    ui->lnUsername->setMaxLength(uname_max_len);
    ui->lnPwd->setMaxLength(pwd_max_len);
}

void FormLogin::InitConnections()
{
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(submitForm()));
}

//�ύ��¼��
void FormLogin::submitForm()
{
    QString title, info;
    username = ui->lnUsername->text();
    password = ui->lnPwd->text();

    //�û�������Ϊ��
    if(username.length() == 0){
        title = CStr2LocalQStr("����");
        info = CStr2LocalQStr("δ��д�û�����");
        QMessageBox::critical(nullptr, title, info);
        ui->lnUsername->setFocus();
        return;
    }
    else if(password.length() == 0){
        title = CStr2LocalQStr("����");
        info = CStr2LocalQStr("δ��д���룡");
        QMessageBox::critical(nullptr, title, info);
        ui->lnPwd->setFocus();
        return;
    }
    //�û����������ʽ����
    else if(ULimits::UNAME_OK != ULimits::checkUname(username) ||
            ULimits::PWD_OK != ULimits::checkPwd(password)){
        title = CStr2LocalQStr("����");
        info = CStr2LocalQStr("�û������������");
        QMessageBox::critical(nullptr, title, info);
        return;
    }
    //�����ݿ��ѯ���û������������
    else if(0){
        title = CStr2LocalQStr("����");
        info = CStr2LocalQStr("�û������������");
        QMessageBox::critical(nullptr, title, info);
        return;
    }
    //�����ʽ��ȷ
#if 0
    title = CStr2LocalQStr("��ʾ");
    info = CStr2LocalQStr("��������");
    QMessageBox::information(nullptr, title, info);
#endif
    qDebug() << CStr2LocalQStr("�������£�");
    //����
    password = QStr2MD5(password);
    emit completed();
}

//��ձ�����
void FormLogin::clearData()
{
    ui->lnUsername->clear();
    ui->lnPwd->clear();
}

QString FormLogin::getUsername()
{
    return this->username;
}

QString FormLogin::getPassword()
{
    return this->password;
}
