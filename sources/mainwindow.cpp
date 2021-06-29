#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QString>
#include <QSpinBox>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <sstream>
#include <iostream>

#include "tools.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    InitMembers();
    InitConnections();
}

MainWindow::~MainWindow()
{
    delete ui;

    delete m_pRegister;
    delete m_pLogin;

    delete this->m_server_sock;
}

void MainWindow::InitMembers()
{
    m_is_connected = 0;
    m_recv_status = 0;
    m_userid = -1;
    m_fileid = -1;

    m_pRegister = new FormRegister;
    m_pLogin = new FormLogin;
    m_pFolder = new FormFolder;

    //ui->spinIP1->setValue(192);
    //ui->spinIP2->setValue(168);
    //ui->spinIP3->setValue(43);
    //ui->spinIP4->setValue(230);
    ui->spinIP1->setValue(10);
    ui->spinIP2->setValue(60);
    ui->spinIP3->setValue(102);
    ui->spinIP4->setValue(252);

    ui->spinPort->setValue(20230);

    InitSocket();

}

void MainWindow::InitConnections()
{
    connect(ui->pbtnConnect, SIGNAL(clicked()), this, SLOT(connectServer()));
    connect(ui->pbtnDisconnect, SIGNAL(clicked()), this, SLOT(disconnectServer()));
    connect(ui->pbtnSend, SIGNAL(clicked()), this, SLOT(sendData()));
    connect(ui->pbtnRecv, SIGNAL(clicked()), this, SLOT(recvData()));

    //打开子窗口
    connect(ui->pbtnRegister, SIGNAL(clicked()), this, SLOT(openRegisterPage()));
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(openLoginPage()));
    connect(ui->pbtnFolder, SIGNAL(clicked()), this, SLOT(openFolderPage()));

    //发送数据
    connect(m_pRegister, SIGNAL(completed()), this, SLOT(sendRegisterData()));
    connect(m_pLogin, SIGNAL(completed()), this, SLOT(sendLoginData()));
    connect(ui->pbtnLogout, SIGNAL(clicked()), this, SLOT(sendLogoutData()));
    connect(m_pFolder, SIGNAL(upfile(const QString &)),
            this, SLOT(sendUpfileData(const QString &)));

    //分段传文件
    connect(ui->pbtnUpfile, SIGNAL(clicked()), this, SLOT(upfileBySeg()));


    connect(m_server_sock, &QTcpSocket::connected, [=](){
        QString title = CStr2LocalQStr("提示");
        QString info = CStr2LocalQStr("连接成功！");
        QMessageBox::information(NULL, title, info);
        m_is_connected = 1;
    });
    connect(m_server_sock, &QTcpSocket::readyRead, [=](){
        QString title = CStr2LocalQStr("提示");
        QString info = CStr2LocalQStr("可以读取数据！");
        QMessageBox::information(NULL, title, info);

        recvData();     //接收数据
    });
    typedef void (QAbstractSocket::*QAbstractSocketErrorSignal)(QAbstractSocket::SocketError);
    connect(m_server_sock, static_cast<QAbstractSocketErrorSignal>(&QTcpSocket::error), [=](){
        QString title = CStr2LocalQStr("提示");
        QString info = CStr2LocalQStr("连接失败！");
        if(1 == m_is_connected)
            info = CStr2LocalQStr("连接断开！");
        QMessageBox::information(NULL, title, info);
        m_is_connected = 0;
        m_userid = -1;
    });
}

void MainWindow::InitSocket()
{
    m_server_sock = new QTcpSocket(this);

    m_server_sock->setReadBufferSize(65536);
}

void MainWindow::connectServer()
{
    int ip_n = 4;
    QSpinBox *pSpinIP[4] = {ui->spinIP1, ui->spinIP2, ui->spinIP3, ui->spinIP4};
    QString ipaddr;
    for(int i = 0; i < ip_n; i++){
        ipaddr += QString::number(pSpinIP[i]->value());
        if(i < ip_n - 1)
            ipaddr += ".";
    }
    int port = ui->spinPort->value();
    qDebug() <<"ip: "<< ipaddr;
    qDebug() <<"port: "<< port;

    m_server_sock->connectToHost(ipaddr, port);

}

void MainWindow::disconnectServer()
{
    m_server_sock->disconnect();
    //server_sock->close();
}

//发送数据
void MainWindow::sendData()
{
    QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QString str;
    //长度
    str += (uchar)(0x00ff & len);
    str += (uchar)((0xff00 & len) >> 8);
    qDebug() <<"slen :" << str.length();
    //内容
    str += content;
    qDebug() << str;
    qDebug() <<"len :" << str.length();
    m_server_sock->write(str.toLatin1());

}

void MainWindow::parseLoginJson(const QString &str)
{
    Json::CharReaderBuilder reader;
    Json::Value recvJson;
    JSONCPP_STRING errs;
    std::stringstream ss(str.toStdString());
    bool res = Json::parseFromStream(reader, ss, &recvJson, &errs);
    if (!res || !errs.empty()) {
        qDebug() << "recv error!";
        return;
    }
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "userid:" << recvJson["userid"].asInt();

    m_userid = recvJson["userid"].asInt();
    m_recv_status = STAT_WAIT;    //清标志位
}

void MainWindow::parseUpfileJson(const QString &str)
{
    Json::CharReaderBuilder reader;
    Json::Value recvJson;
    JSONCPP_STRING errs;
    std::stringstream ss(str.toStdString());
    bool res = Json::parseFromStream(reader, ss, &recvJson, &errs);
    if (!res || !errs.empty()) {
        qDebug() << "recv error!";
        return;
    }
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "fileid:" << recvJson["fileid"].asInt();

    m_fileid = recvJson["fileid"].asInt();
    m_recv_status = STAT_WAIT;    //清标志位
}

//接收数据
void MainWindow::recvData()
{
    QByteArray str = m_server_sock->readAll();
    //长度
    unsigned short len, len0, len1;
    len0 = (unsigned short)str[0];
    len1 = (unsigned short)str[1];
    len = len0 + (len1 << 8);
    qDebug() << "recv len: "<< len;
    //内容
    str = str.mid(2, len);
    qDebug() << "read : "<< str;
    qDebug() << "len : "<< str.length();

    ui->txtRecv->setText(str);

    //登录时，记录userid
    if(STAT_LOGIN == m_recv_status){
        parseLoginJson(str);
    }
    else if(STAT_UPFILE == m_recv_status){
        parseUpfileJson(str);
    }
}

//打开注册页面s
void MainWindow::openRegisterPage()
{
    m_pRegister->show();
}

//打开登录页面
void MainWindow::openLoginPage()
{
    m_pLogin->show();
}

//打开目录页面
void MainWindow::openFolderPage()
{
    QString root_dir = "E:/test";
    m_pFolder->setRootDir(root_dir);
    m_pFolder->InitFolderTree();
    m_pFolder->show();
}

//注册
void MainWindow::sendRegisterData()
{
    qDebug() <<"register...";
    QString usname = m_pRegister->getUsername();
    QString pwd = m_pRegister->getPassword();
    qDebug() <<"username: "<< usname;
    qDebug() <<"password: "<< pwd;

    Json::Value sendJson;
    sendJson["function"] = "register";
    sendJson["username"] = usname.toStdString();
    sendJson["password"] = pwd.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    //pRegister->clearData();
    m_pRegister->close();
    //m_username = usname;
    //setUsername();

    m_recv_status = STAT_REGISTER;
}

//登录
void MainWindow::sendLoginData()
{
    qDebug() <<"login...";
    QString usname = m_pLogin->getUsername();
    QString pwd = m_pRegister->getPassword();
    qDebug() <<"username: "<< usname;
    qDebug() <<"password: "<< pwd;

    Json::Value sendJson;
    sendJson["function"] = "login";
    sendJson["username"] = usname.toStdString();
    sendJson["password"] = pwd.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    //pLogin->clearData();
    m_pLogin->close();
    m_username = usname;
    setUsername();

    m_recv_status = STAT_LOGIN;
}

//退出登录
void MainWindow::sendLogoutData()
{
    //未登录
    //if(userid < 0)
    //    return;
    qDebug() <<"logout...";
    qDebug() <<"username: "<< m_username;
    qDebug() <<"userid: "<< m_userid;

    Json::Value sendJson;
    sendJson["function"] = "logout";
    sendJson["userid"] = m_userid;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_userid = -1;    //置非法id

    m_recv_status = STAT_WAIT;
}

//分段上传文件
void MainWindow::upfileBySeg()
{
    if(m_file_path.length() == 0){
        QString title = CStr2LocalQStr("错误");
        QString info = CStr2LocalQStr("未选择文件！");
        QMessageBox::critical(NULL, title, info);
        return;
    }
    QFileInfo file_info(m_file_path);
    qint64 total_len = file_info.size();
    qDebug() << CStr2LocalQStr("文件长度：") + QString::number(total_len);

    const int buf_len = 4096;
    int one_send_len = buf_len;
    static qint64 start_bit = 0;
    qint64 remain_len = total_len - start_bit;
    if(remain_len > 0){
        if(one_send_len > remain_len)
            one_send_len = int(remain_len);
        sendUpfilesegData(m_file_path, start_bit, one_send_len);

        start_bit += one_send_len;
        remain_len -= one_send_len;

        qDebug() << CStr2LocalQStr("已发送") << QString::number(start_bit) << CStr2LocalQStr("字节，剩余")
                 << QString::number(remain_len) << CStr2LocalQStr("字节");
    }
    else {
        qDebug() << CStr2LocalQStr("上传完毕！");

        QString title = CStr2LocalQStr("提示");
        QString info = CStr2LocalQStr("文件上传完毕！");
        QMessageBox::information(NULL, title, info);

        ui->txtSend->clear();
        m_file_path = "";
        start_bit = 0;
    }
}

//上传文件
void MainWindow::sendUpfileData(const QString &file_path)
{
    QString full_path = m_pFolder->getRootDir() + "/" + file_path;
    m_file_path = full_path;

    QFileInfo file_info(full_path);
    quint64 file_size = file_info.size();
    if(file_size <= 0){
        QString title = CStr2LocalQStr("错误");
        QString info = CStr2LocalQStr("不能上传空文件！");
        QMessageBox::critical(NULL, title, info);
        ui->txtSend->clear();
        return;
    }
    QString file_md5 = getFileMD5(full_path);

    qDebug() <<"upfile...";
    qDebug() <<"userid: "<< m_userid;
    qDebug() <<"path: "<< file_path;
    qDebug() <<"fullpath: "<< full_path;
    qDebug() <<"md5: "<< file_md5;
    qDebug() <<"length: "<< file_size;


    Json::Value sendJson;
    sendJson["function"] = "upfile";
    sendJson["userid"] = m_userid;
    sendJson["path"] = file_path.toStdString();
    sendJson["md5"] = file_md5.toStdString();
    sendJson["length"] = file_size;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_UPFILE;
}

//发送文件片段
void MainWindow::sendUpfilesegData(const QString &file_path, qint64 start_bit, int len)
{
    QFile file_in(file_path);
    if(!file_in.open(QFile::ReadOnly)){
        qDebug() << CStr2LocalQStr("文件打开失败！");
        return;
    }
    if(start_bit >= file_in.size()){
        qDebug() << CStr2LocalQStr("传输位置错误！") + "start_bit=" + QString::number(start_bit);
        return;
    }

    file_in.seek(start_bit);    //移动文件指针，
    QByteArray seg_ba = file_in.read(len);     //读取文件
    file_in.close();        //关闭文件

    QString seg_content(seg_ba);
    QString seg_md5 = QStr2MD5(seg_content);

    qDebug() <<"upfileseg...";
    qDebug() <<"fileid: "<< m_fileid;
    qDebug() <<"md5: "<< seg_md5;
    qDebug() <<"content: "<< seg_content;

    Json::Value sendJson;
    sendJson["function"] = "upfileseg";
    sendJson["fileid"] = m_fileid;
    sendJson["md5"] = seg_md5.toStdString();
    sendJson["content"] = seg_content.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);
}

void MainWindow::setUsername()
{
    ui->lnUsername->setText(m_username);
}

//窗口关闭
void MainWindow::closeEvent(QCloseEvent *event)
{
    //若用户为登录状态，发送退出信号
    if(m_userid >= 0){
        sendLogoutData();
        sendData();
    }
    else {
        sendLogoutData();
        qDebug() << CStr2LocalQStr("用户已退出");
    }
    event->accept();
    qDebug() << CStr2LocalQStr("主窗口关闭！");
}
