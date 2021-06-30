#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTime>

#include <QString>
#include <QSpinBox>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QByteArray>
#include <QTextCodec>
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

    clearUserid();
    clearUpfile();

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
    //connect(ui->pbtnSend, SIGNAL(clicked()), this, SLOT(sendData()));
    connect(ui->pbtnRecv, SIGNAL(clicked()), this, SLOT(recvData()));

    //打开子窗口
    connect(ui->pbtnRegister, SIGNAL(clicked()), this, SLOT(openRegisterPage()));
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(openLoginPage()));
    connect(ui->pbtnFolder, SIGNAL(clicked()), this, SLOT(openFolderPage()));

    //发送数据
    connect(m_pRegister, SIGNAL(completed()), this, SLOT(sendDataRegister()));
    connect(m_pLogin, SIGNAL(completed()), this, SLOT(sendDataLogin()));
    connect(ui->pbtnLogout, SIGNAL(clicked()), this, SLOT(sendDataLogout()));

    //上传文件
    connect(m_pFolder, SIGNAL(upfile(const QString &)), this, SLOT(sendDataUpfile(const QString &)));
    //删除文件
    connect(m_pFolder, SIGNAL(rmfile(const QString &)), this, SLOT(sendDataRmfile(const QString &)));
    //创建目录
    connect(m_pFolder, SIGNAL(mkdir(const QString &)), this, SLOT(sendDataMkdir(const QString &)));
    //删除目录
    connect(m_pFolder, SIGNAL(rmdir(const QString &)), this, SLOT(sendDataRmdir(const QString &)));
    //分段传文件
    connect(ui->pbtnUpfile, SIGNAL(clicked()), this, SLOT(upfileBySeg()));


    connect(m_server_sock, &QTcpSocket::connected, [=](){
        MyMessageBox::information("提示", "连接成功！");
        m_is_connected = 1;
    });
    connect(m_server_sock, &QTcpSocket::readyRead, [=](){
#if 0
        MyMessageBox::information("提示", "可以读取数据！");
#endif
        recvData();     //接收数据
    });
    typedef void (QAbstractSocket::*QAbstractSocketErrorSignal)(QAbstractSocket::SocketError);
    connect(m_server_sock, static_cast<QAbstractSocketErrorSignal>(&QTcpSocket::error), [=](){
        if(1 == m_is_connected)
            MyMessageBox::information("提示", "连接断开！");
        else
            MyMessageBox::information("提示", "连接失败！");
        m_is_connected = 0;
        clearUserid();
        clearUsername();
    });
}

void MainWindow::setUsername()
{
    ui->lnUsername->setText(m_username);
}

void MainWindow::clearUsername()
{
    m_username.clear();
    ui->lnUsername->setText(CStr2LocalQStr("未登录"));
}

void MainWindow::clearUserid()
{
    m_userid = -1;
}

void MainWindow::clearUpfile()
{
    m_fileid = -1;
    m_start_bit = 0;
    m_file_path.clear();
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
void MainWindow::sendData(const QString &content)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QString str;
    //长度
    str += (uchar)(0x00ff & len);
    str += (uchar)((0xff00 & len) >> 8);
#ifdef DEBUG_UPSEG
    qDebug() <<"slen :" << str.length();
#endif
    //内容
    str += content;
    //qDebug() << str;
    //qDebug() <<"len :" << str.length();
    m_server_sock->write(str.toLatin1());

}

//解析从服务端收到的消息
void MainWindow::parseJson(const QString &str)
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
#ifdef DEBUG_UPSEG
    qDebug() << "function:" << recvJson["function"].asCString();
#endif
    //根据function进行解析
    if(recvJson["function"] == "register"){
        parseJsonRegister(recvJson);
    }
    else if(recvJson["function"] == "login"){
        parseJsonLogin(recvJson);
    }
    else if(recvJson["function"] == "upfile"){
        parseJsonUpfile(recvJson);
    }
    else if(recvJson["function"] ==  "upfileseg"){
        parseJsonUpfileseg(recvJson);
    }
    else if(recvJson["function"] ==  "rmfile"){
        parseJsonRmfile(recvJson);
    }
    else if(recvJson["function"] ==  "mkdir"){
        parseJsonMkdir(recvJson);
    }
    else if(recvJson["function"] ==  "rmdir"){
        parseJsonRmdir(recvJson);
    }
}

//注册
void MainWindow::parseJsonRegister(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();

    int status = recvJson["status"].asInt();
    if(1 == status){
        MyMessageBox::information("提示", "注册成功！");
    }
    else {
        MyMessageBox::critical("错误", "注册失败！该用户名已被注册！");
    }
    m_recv_status = STAT_WAIT;    //清标志位
}

//登录
void MainWindow::parseJsonLogin(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "userid:" << recvJson["userid"].asInt();

    int status = recvJson["status"].asInt();
    if(1 == status){
        MyMessageBox::information("提示", "登录成功！");
        m_userid = recvJson["userid"].asInt();
        setUsername();
    }
    else {
        MyMessageBox::critical("错误", "登录失败！用户名或密码不正确！");
        clearUserid();
        clearUsername();
    }
    m_recv_status = STAT_WAIT;    //清标志位
}

//上传文件
void MainWindow::parseJsonUpfile(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "fileid:" << recvJson["fileid"].asInt();
    qDebug() << "startbit:" << recvJson["startbit"].asInt();

    int status = recvJson["status"].asInt();
    if(0 == status){    //已同步，无需上传
        MyMessageBox::information("提示", "已同步，无需上传！");
        clearUpfile();
        return;
    }
    m_fileid = recvJson["fileid"].asInt();
    m_start_bit = recvJson["startbit"].asInt();
    m_recv_status = STAT_WAIT;    //清标志位
    //开始上传
    upfileBySeg();
}

//上传文件片段
void MainWindow::parseJsonUpfileseg(const Json::Value &recvJson)
{
#ifdef DEBUG_UPSEG
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "finish:" << recvJson["finish"].asInt();
#endif
    int segtrue = recvJson["segtrue"].asInt();
    int finish = recvJson["finish"].asInt();

    if(!segtrue){       //本段上传失败
        MyMessageBox::critical("提示", "传输出错！");
        clearUpfile();
        return;
    }
    else if(finish) {        //上传完成
        MyMessageBox::information("提示", "上传完成！");
        clearUpfile();
        return;
    }
    //## 发文件片段，不清标志位
    //m_recv_status = STAT_WAIT;
    //本段上传成功，传下一段
    upfileBySeg();
}

//删除文件
void MainWindow::parseJsonRmfile(const Json::Value &recvJson)
{

}

//建立目录
void MainWindow::parseJsonMkdir(const Json::Value &recvJson)
{

}

//删除目录
void MainWindow::parseJsonRmdir(const Json::Value &recvJson)
{

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
    //内容
    str = str.mid(2, len);

#ifdef DEBUG_UPSEG
    qDebug() << "recv len: "<< len;
    qDebug() << "read : "<< str;
    qDebug() << "len : "<< str.length();
#endif
    if(m_recv_status != STAT_UPSEG)
        ui->txtRecv->setText(str);

    parseJson(str);

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
void MainWindow::sendDataRegister()
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

    //pRegister->clearData();   //清除信息
    //m_pRegister->close();     //关闭窗口

    m_recv_status = STAT_REGISTER;
    sendData(sendbuf);     //发送数据
}

//登录
void MainWindow::sendDataLogin()
{
    qDebug() <<"login...";
    QString usname = m_pLogin->getUsername();
    QString pwd = m_pLogin->getPassword();
    qDebug() <<"username: "<< usname;
    qDebug() <<"password: "<< pwd;

    Json::Value sendJson;
    sendJson["function"] = "login";
    sendJson["username"] = usname.toStdString();
    sendJson["password"] = pwd.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    //pLogin->clearData();  //清除信息
    //m_pLogin->close();    //关闭窗口
    m_username = usname;

    m_recv_status = STAT_LOGIN;
    sendData(sendbuf);     //发送数据
}

//退出登录
void MainWindow::sendDataLogout()
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

    clearUserid();    //置非法id

    m_recv_status = STAT_WAIT;
    sendData(sendbuf);     //发送数据
}

//分段上传文件
void MainWindow::upfileBySeg()
{
    if(m_file_path.length() == 0){
        MyMessageBox::critical("错误", "未选择文件！");
        m_recv_status = STAT_WAIT;
        return;
    }
    QFileInfo file_info(m_file_path);
    qint64 total_len = file_info.size();
#ifdef DEBUG_UPSEG
    qDebug() << CStr2LocalQStr("文件长度：") + QString::number(total_len);
#endif
    const int buf_len = 4096;
    int one_send_len = buf_len;
    qint64 remain_len = total_len - m_start_bit;

    static QTime qtime_0 = QTime::currentTime();
    QTime qtime_1 = QTime::currentTime();
    if(qtime_1.second() != qtime_0.second()){
        qtime_0 = qtime_1;
        qDebug() << CStr2LocalQStr("") << QString::number(m_start_bit)
                 << CStr2LocalQStr("字节，剩余") << QString::number(remain_len)
                 << CStr2LocalQStr("字节");
    }

    if(remain_len > 0){
        if(one_send_len > remain_len)
            one_send_len = int(remain_len);
        sendDataUpfileseg(m_file_path, m_start_bit, one_send_len);

        //m_start_bit += one_send_len;
        //remain_len -= one_send_len;
        /*
        qDebug() << CStr2LocalQStr("已发送") << QString::number(m_start_bit)
                 << CStr2LocalQStr("字节，剩余") << QString::number(remain_len)
                 << CStr2LocalQStr("字节");
        */
    m_recv_status = STAT_UPSEG;
    }
    else {
        clearUpfile();
        m_recv_status = STAT_WAIT;
    }
}

//上传文件
void MainWindow::sendDataUpfile(const QString &file_path)
{
    QString full_path = m_pFolder->getRootDir() + "/" + file_path;
    m_file_path = full_path;

    QFileInfo file_info(full_path);
    quint64 file_size = file_info.size();
    if(file_size <= 0){
        MyMessageBox::critical("错误", "不能上传空文件！");
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
    sendData(sendbuf);     //发送数据
}

//发送文件片段
void MainWindow::sendDataUpfileseg(const QString &file_path, qint64 start_bit, int len)
{
    QFile file_in(file_path);
    if(!file_in.open(QIODevice::ReadOnly)){
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

    m_start_bit += seg_ba.length();

    //QByteArray是字节流，不能直接转QString，否则'\0'之后都会被过滤
    QTextCodec *codec = QTextCodec::codecForName("KOI8-R");

    QString seg_content = codec->toUnicode(seg_ba);
    //QString seg_md5 = QStr2MD5(seg_content);
    QString seg_md5 = QBa2MD5(seg_ba);

#ifdef DEBUG_UPSEG
    qDebug() <<"***************************";
    qDebug() << "len="<<len <<", read_len:" << seg_ba.length();
#endif
    int str_len = seg_content.toStdU32String().length();

    QString msg = "len=" + QString::number(len) + ", read_len=" + QString::number(seg_content.length())
            + ", str_len=" + QString::number(str_len);
    //QMessageBox::information(nullptr, "info", msg);

#ifdef DEBUG_UPSEG
    qDebug() <<"***************************";

    qDebug() <<"upfileseg...";
    qDebug() <<"fileid: "<< m_fileid;
    qDebug() <<"md5: "<< seg_md5;
    //qDebug() <<"content: "<< seg_content;
#endif
    Json::Value sendJson;
    sendJson["function"] = "upfileseg";
    sendJson["fileid"] = m_fileid;
    sendJson["md5"] = seg_md5.toStdString();
    //sendJson["content"] = seg_content.toStdString();
    //### 用int数组存字符
    for(int i = 0; i < len; i++){
        sendJson["content"][i] = int(seg_ba[i]);
    }
    QString sendbuf = sendJson.toStyledString().data();
    //ui->txtSend->setText(sendbuf);

    sendData(sendbuf);     //发送数据
}

//删除文件
void MainWindow::sendDataRmfile(const QString &file_path)
{
    Json::Value sendJson;
    sendJson["function"] = "rmfile";
    sendJson["path"] = file_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_RMFILE;
    sendData(sendbuf);     //发送数据
}

//创建目录
void MainWindow::sendDataMkdir(const QString &dir_path)
{
    Json::Value sendJson;
    sendJson["function"] = "mkdir";
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_MKDIR;
    sendData(sendbuf);     //发送数据
}

//删除目录
void MainWindow::sendDataRmdir(const QString &dir_path)
{
    Json::Value sendJson;
    //sendJson["function"] = "rmdir";
    sendJson["function"] = "rmfile";    //删文件和删目录命令合并
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_RMDIR;
    sendData(sendbuf);     //发送数据
}

//窗口关闭
void MainWindow::closeEvent(QCloseEvent *event)
{
    //若用户为登录状态，发送退出信号
    if(m_userid >= 0){
        sendDataLogout();
    }
    else {
        sendDataLogout();
        qDebug() << CStr2LocalQStr("用户已退出");
    }
    event->accept();
    qDebug() << CStr2LocalQStr("主窗口关闭！");
}
