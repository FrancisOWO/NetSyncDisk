#include "form_register.h"
#include "ui_form_register.h"
#include <QMessageBox>
#include <QDebug>

FormRegister::FormRegister(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormRegister)
{
    ui->setupUi(this);

    InitMembers();
    InitConnections();
}

FormRegister::~FormRegister()
{
    delete ui;
}

void FormRegister::InitMembers()
{
    //Ĭ�ϵ��û��������룬�������
    ui->lnUsername->setText("test_user_1");
    ui->lnPwd1->setText("123abcde_");
    ui->lnPwd2->setText("123abcde_");

    //�������볤��
    const int uname_max_len = ULimits::uname_max_len;
    const int pwd_max_len = ULimits::pwd_max_len;
    ui->lnUsername->setMaxLength(uname_max_len);
    ui->lnPwd1->setMaxLength(pwd_max_len);
    ui->lnPwd2->setMaxLength(pwd_max_len);
}

void FormRegister::InitConnections()
{
    connect(ui->pbtnRegister, SIGNAL(clicked()), this, SLOT(submitForm()));
}

int FormRegister::checkUname()
{
    QString title, info;
    //���ȼ��
    const int min_len = ULimits::uname_min_len;
    const int max_len = ULimits::uname_max_len;
    if(!ULimits::checkUnameLen(username)){
        title = CStr2LocalQStr("����");
        info = CStr2LocalQStr("�û�������ӦΪ") + QString::number(min_len)
               + "-" +  QString::number(max_len) + CStr2LocalQStr("λ��");
        QMessageBox::critical(nullptr, title, info);
        return ULimits::UNAME_ERRLEN;
    }
    //�ַ����
    bool chk_ok = ULimits::checkUnameCh(username);
    if(!chk_ok){
        MyMessageBox::critical("����", "�û������ܺ������֡���ĸ���»���������ַ���");
        return ULimits::UNAME_ERRCH;
    }
    return ULimits::UNAME_OK;
}

int FormRegister::checkPwd()
{
    QString title, info;
    //���ȼ��
    const int min_len = ULimits::pwd_min_len;
    const int max_len = ULimits::pwd_max_len;
    if(!ULimits::checkPwdLen(password)){
        title = CStr2LocalQStr("����");
        info = CStr2LocalQStr("���볤��ӦΪ") + QString::number(min_len)
               + "-" +  QString::number(max_len) + CStr2LocalQStr("λ��");
        QMessageBox::critical(nullptr, title, info);
        return ULimits::PWD_ERRLEN;
    }

    //�ַ����
    bool chk_ok = ULimits::checkPwdCh(password);
    if(!chk_ok){
        MyMessageBox::critical("����", "�������ͬʱ��������/��ĸ/�»��ߣ��Ҳ��ܰ��������ַ���");
        return  ULimits::PWD_ERRCH;
    }

    return ULimits::PWD_OK;
}

//�ύע���
void FormRegister::submitForm()
{
    QString title, info;
    QString pwd1, pwd2;
    username = ui->lnUsername->text();
    pwd1 = ui->lnPwd1->text();
    pwd2 = ui->lnPwd2->text();
    password = pwd1;

    //�û�������Ϊ��
    if(username.length() == 0){
        MyMessageBox::critical("����", "δ��д�û�����");
        ui->lnUsername->setFocus();
        return;
    }
    else if(pwd1.length() == 0){
        MyMessageBox::critical("����", "δ��д���룡");
        ui->lnPwd1->setFocus();
        return;
    }
    else if(pwd2.length() == 0){
        MyMessageBox::critical("����", "δ��д�ظ����룡");
        ui->lnPwd2->setFocus();
        return;
    }
    //�û�����ʽ����
    else if(ULimits::UNAME_OK != checkUname()){
        return;
    }
    //�����ʽ����
    else if(ULimits::PWD_OK != checkPwd()){
        return;
    }
    //�������벻һ��
    else if(pwd1 != pwd2){
        MyMessageBox::critical("����", "������������벻һ�£�");
        return;
    }
    qDebug() << CStr2LocalQStr("�������£�");
    //����
    password = QStr2MD5(password);
    emit completed();
}

//��ձ�����
void FormRegister::clearData()
{
    ui->lnUsername->clear();
    ui->lnPwd1->clear();
    ui->lnPwd2->clear();
}

QString FormRegister::getUsername()
{
    return this->username;
}

QString FormRegister::getPassword()
{
    return this->password;
}
