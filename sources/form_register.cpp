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
    //默认的用户名和密码，方便测试
    ui->lnUsername->setText("test_user_1");
    ui->lnPwd1->setText("123abcde_");
    ui->lnPwd2->setText("123abcde_");

    //限制输入长度
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
    //长度检查
    const int min_len = ULimits::uname_min_len;
    const int max_len = ULimits::uname_max_len;
    if(!ULimits::checkUnameLen(username)){
        title = CStr2LocalQStr("错误");
        info = CStr2LocalQStr("用户名长度应为") + QString::number(min_len)
               + "-" +  QString::number(max_len) + CStr2LocalQStr("位！");
        QMessageBox::critical(nullptr, title, info);
        return ULimits::UNAME_ERRLEN;
    }
    //字符检查
    bool chk_ok = ULimits::checkUnameCh(username);
    if(!chk_ok){
        MyMessageBox::critical("错误", "用户名不能含有数字、字母、下划线以外的字符！");
        return ULimits::UNAME_ERRCH;
    }
    return ULimits::UNAME_OK;
}

int FormRegister::checkPwd()
{
    QString title, info;
    //长度检查
    const int min_len = ULimits::pwd_min_len;
    const int max_len = ULimits::pwd_max_len;
    if(!ULimits::checkPwdLen(password)){
        title = CStr2LocalQStr("错误");
        info = CStr2LocalQStr("密码长度应为") + QString::number(min_len)
               + "-" +  QString::number(max_len) + CStr2LocalQStr("位！");
        QMessageBox::critical(nullptr, title, info);
        return ULimits::PWD_ERRLEN;
    }

    //字符检查
    bool chk_ok = ULimits::checkPwdCh(password);
    if(!chk_ok){
        MyMessageBox::critical("错误", "密码必须同时包含数字/字母/下划线，且不能包含其他字符！");
        return  ULimits::PWD_ERRCH;
    }

    return ULimits::PWD_OK;
}

//提交注册表单
void FormRegister::submitForm()
{
    QString title, info;
    QString pwd1, pwd2;
    username = ui->lnUsername->text();
    pwd1 = ui->lnPwd1->text();
    pwd2 = ui->lnPwd2->text();
    password = pwd1;

    //用户名输入为空
    if(username.length() == 0){
        MyMessageBox::critical("错误", "未填写用户名！");
        ui->lnUsername->setFocus();
        return;
    }
    else if(pwd1.length() == 0){
        MyMessageBox::critical("错误", "未填写密码！");
        ui->lnPwd1->setFocus();
        return;
    }
    else if(pwd2.length() == 0){
        MyMessageBox::critical("错误", "未填写重复密码！");
        ui->lnPwd2->setFocus();
        return;
    }
    //用户名格式出错
    else if(ULimits::UNAME_OK != checkUname()){
        return;
    }
    //密码格式出错
    else if(ULimits::PWD_OK != checkPwd()){
        return;
    }
    //两次密码不一致
    else if(pwd1 != pwd2){
        MyMessageBox::critical("错误", "两次输入的密码不一致！");
        return;
    }
    qDebug() << CStr2LocalQStr("输入框更新！");
    //加密
    password = QStr2MD5(password);
    emit completed();
}

//清空表单数据
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
