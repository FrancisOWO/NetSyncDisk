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
    //默认的用户名和密码，方便测试
    ui->lnUsername->setText("test_user_1");
    ui->lnPwd->setText("123abcde_");

    //限制输入长度
    const int uname_max_len = ULimits::uname_max_len;
    const int pwd_max_len = ULimits::pwd_max_len;
    ui->lnUsername->setMaxLength(uname_max_len);
    ui->lnPwd->setMaxLength(pwd_max_len);
}

void FormLogin::InitConnections()
{
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(submitForm()));
}

//提交登录表单
void FormLogin::submitForm()
{
    QString title, info;
    username = ui->lnUsername->text();
    password = ui->lnPwd->text();

    //用户名输入为空
    if(username.length() == 0){
        MyMessageBox::critical("错误", "未填写用户名！");
        ui->lnUsername->setFocus();
        return;
    }
    else if(password.length() == 0){
        MyMessageBox::critical("错误", "未填写密码！");
        ui->lnPwd->setFocus();
        return;
    }
    //用户名格式出错
    else if(ULimits::UNAME_OK != ULimits::checkUname(username)){
        MyMessageBox::critical("错误", "登录失败！用户不存在！");
    }
    //密码格式错误
    else if (ULimits::PWD_OK != ULimits::checkPwd(password)){
        MyMessageBox::critical("错误", "登录失败！密码错误！");
        return;
    }
    //输入格式正确
    qDebug() << CStr2LocalQStr("输入框更新！");
    //加密
    password = QStr2MD5(password);
    emit completed();
}

//清空表单数据
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
