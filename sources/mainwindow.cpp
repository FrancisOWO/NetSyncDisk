#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QString>
#include <QStringList>
#include <QByteArray>

#include <QDateTime>
#include <QTime>
#include <QAbstractSocket>

#include <QSpinBox>
#include <QMessageBox>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>

#include <QDebug>

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
using namespace std;

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
    delete m_pFolder;

    disconnectServer();
    DestroySocket();
}

void MainWindow::InitMembers()
{
    m_recv_status = 0;

    InitLogDir();   //创建日志目录

    setConnectStatus(CONN_NO);
    clearUser();    //清空用户信息
    clearUpfile();

    setNotLoginUI();    //设置未登录界面

    m_pRegister = new FormRegister;
    m_pLogin = new FormLogin;
    m_pFolder = new FormFolder;

    //ui->spinIP1->setValue(192);
    //ui->spinIP2->setValue(168);
    //ui->spinIP3->setValue(43);
    //ui->spinIP4->setValue(230);
    ui->progStatus->setValue(0);
    //IP
    ui->spinIP1->setValue(10);
    ui->spinIP2->setValue(60);
    ui->spinIP3->setValue(102);
    ui->spinIP4->setValue(252);
    //端口
    ui->spinPort->setValue(20230);

    InitSocket();

    //自动连接服务器（登录后再连接）
    //connectServer();
}

void MainWindow::InitConnections()
{
    //Socket连接
    connect(ui->pbtnConnect, SIGNAL(clicked()), this, SLOT(connectServer()));
    connect(ui->pbtnDisconnect, SIGNAL(clicked()), this, SLOT(disconnectServer()));

    connect(ui->pbtnSend, SIGNAL(clicked()), this, SLOT(sendDataFromBox()));
    connect(ui->pbtnRecv, SIGNAL(clicked()), this, SLOT(recvDataFromBox()));

    //打开子窗口
    connect(ui->pbtnRegister, SIGNAL(clicked()), this, SLOT(openRegisterPage()));
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(openLoginPage()));
    connect(ui->pbtnFolder, SIGNAL(clicked()), this, SLOT(openFolderPage()));

    //发送数据
    //注册
    connect(m_pRegister, SIGNAL(completed()), this, SLOT(sendDataRegister()));
    //登录
    connect(m_pLogin, SIGNAL(completed()), this, SLOT(sendDataLogin()));
    //登出
    connect(ui->pbtnLogout, SIGNAL(clicked()), this, SLOT(sendDataLogout()));
    //请求目录
    connect(ui->pbtnAskPath, SIGNAL(clicked()), this, SLOT(sendDataAskAllPath()));

    //绑定目录
    connect(m_pFolder, SIGNAL(banded(const QString &, const QString &)),
            this, SLOT(WriteBandLog(const QString &, const QString &)));

    //上传文件
    connect(m_pFolder, SIGNAL(upfile(const QString &)), this, SLOT(sendDataUpfile(const QString &)));
    //删除文件
    connect(m_pFolder, SIGNAL(rmfile(const QString &)), this, SLOT(sendDataRmfile(const QString &)));
    //创建目录
    connect(m_pFolder, SIGNAL(mkdir(const QString &)), this, SLOT(sendDataMkdir(const QString &)));
    //删除目录
    connect(m_pFolder, SIGNAL(rmdir(const QString &)), this, SLOT(sendDataRmdir(const QString &)));
    //分段传文件
    //connect(ui->pbtnUpfile, SIGNAL(clicked()), this, SLOT(upfileBySeg()));
    //下载文件
    connect(m_pFolder, SIGNAL(downfile(const QString &)), this, SLOT(sendDataDownfile(const QString &)));

#if 0
    connect(m_server_sock, &QTcpSocket::connected, [=](){
        WriteConnectLog("连接成功");
        //MyMessageBox::information("提示", "连接成功！");
        setConnectStatus(CONN_OK);
    });
#endif
    connect(m_server_sock, &QTcpSocket::readyRead, [=](){
        //MyMessageBox::information("提示", "可以读取数据！");
        recvData();     //接收数据
    });
    typedef void (QAbstractSocket::*QAbstractSocketErrorSignal)(QAbstractSocket::SocketError);
    connect(m_server_sock, static_cast<QAbstractSocketErrorSignal>(&QTcpSocket::error), [=](){
        if(1 == m_is_connected){
            WriteConnectLog("连接断开");
            qDebug() << CStr2LocalQStr("连接断开！");
            //MyMessageBox::critical("错误", "连接断开！");
        }
        else {
            WriteConnectLog("连接失败");
            qDebug() << CStr2LocalQStr("连接失败！");
            //MyMessageBox::critical("错误", "连接失败！");
        }
        setConnectStatus(CONN_NO);
        clearUser();
    });
}

bool MainWindow::isConnected()
{
    return m_is_connected;
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

void MainWindow::clearUser()
{
    clearUserid();
    clearUsername();
}

void MainWindow::clearUpfile()
{
    m_fileid = -1;
    m_startbit = 0;

    qDebug() <<"!!! clear path:" << m_filepath;
    m_filepath.clear();
}

void MainWindow::InitLogDir()
{
    QStringList dir_list = {"userinfo", "log"};
    for(int i = 0; i < dir_list.length(); i++){
        //QString path = dir_list[i];
        //createDir(path);
        QDir temp_dir(dir_list[i]);
        QString path = temp_dir.absolutePath();
        qDebug() <<"abs path: "<< path;
        createDir(path);
    }
    return;
}

void MainWindow::hideSubWin()
{
    m_pFolder->hide();
    m_pLogin->hide();
    m_pRegister->hide();
}

void MainWindow::InitSocket()
{
    m_server_sock = new QTcpSocket(this);
    m_server_sock->setReadBufferSize(65536);
}

void MainWindow::DestroySocket()
{
    m_server_sock->close();
    delete  m_server_sock;
}

QString MainWindow::getUrecPath()
{
    return "userinfo/" + QString::number(m_userid) + ".connect.rec";
}

QString MainWindow::getConnLogPath()
{
    return "log/connect.log";
}

int MainWindow::getRealpathLen(const QString &path)
{
    int base_len = m_pFolder->getRootDir().length() + 1;
    int len = path.length() - base_len - 4;   //减去".tmp"
    if(len <= 0)
        return 0;
    return len;
}

//判断用户是否登录
bool MainWindow::isLoginUser()
{
    bool is_login = (m_userid >= 0);
    if(!is_login){
        qDebug() << "ERROR: Not Login!";
        //MyMessageBox::critical("错误", "用户未登录！");
    }
    is_login = 1;
    return is_login;
}

void MainWindow::setConnectStatus(int status)
{
    QString str;
    if(status == CONN_NO){
        str = CStr2LocalQStr("未连接");
        m_is_connected = 0;
    }
    else if(status == CONN_ING){
        str = CStr2LocalQStr("连接中......");
        m_is_connected = 0;
    }
    else if(status == CONN_OK){
        str = CStr2LocalQStr("已连接");
        m_is_connected = 1;
    }
    ui->lnConStatus->setText(str);
}

void MainWindow::setLoginUI()
{
    ui->pbtnLogin->setVisible(false);
    ui->pbtnRegister->setVisible(false);
    ui->pbtnLogout->setVisible(true);
    ui->pbtnFolder->setVisible(true);
    ui->pbtnAskPath->setVisible(true);
}

void MainWindow::setNotLoginUI()
{
    ui->pbtnLogin->setVisible(true);
    ui->pbtnRegister->setVisible(true);
    ui->pbtnLogout->setVisible(false);
    ui->pbtnFolder->setVisible(false);
    ui->pbtnAskPath->setVisible(false);
}

void MainWindow::renameFileWithoutTmp()
{
    int newpath_len = m_filepath.length() - 4;  //去掉".tmp"
    QString new_filepath = m_filepath.mid(0, newpath_len);
    //改名前，过滤以防止监视
    m_pFolder->m_temp_path = new_filepath;
    qDebug() <<"rename "<< m_filepath << " "<< new_filepath;
    if(!QFile::rename(m_filepath, new_filepath)){    //改名
        QString log_str = CStr2LocalQStr("同名文件覆盖[userid:") + QString::number(m_userid) + "] ";
        log_str += new_filepath;
        WriteConnectLog(log_str);

        //MyMessageBox::warning("警告", "存在冲突，覆盖同名文件！");
        QFile::remove(new_filepath);
        QFile::rename(m_filepath, new_filepath);
    }
    m_filepath = new_filepath;
    m_pFolder->AddWatchPath(m_filepath);    //手动添加监视

}

void MainWindow::WriteConnectLog(const char *str)
{
    WriteConnectLog(CStr2LocalQStr(str));
}

void MainWindow::WriteConnectLog(const QString &str)
{
    QString filename = getConnLogPath();
    QFile qfout(filename);
    if(!qfout.open(QFile::ReadWrite)){
        MyMessageBox::critical("错误","connect文件打开失败！");
        return;
    }
    //加时间戳
    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QByteArray write_ba = "[" + time_str.toLocal8Bit() + "]";
    write_ba += str.toLocal8Bit();
    write_ba += "\n";

    qfout.seek(qfout.size());
    qfout.write(write_ba, write_ba.length());
    qfout.close();
}

void MainWindow::WriteConnectRec(const QString &local_dir, const QString &remote_dir)
{
    QString file_path = getUrecPath();
    QFile file_out(file_path);
    if(!file_out.open(QFile::WriteOnly)){
        QString str = CStr2LocalQStr("绑定文件打开失败[userid:") + QString::number(m_userid) + "]";
        WriteConnectLog(str);
        return;
    }
    QString band_rec = local_dir + "|" + remote_dir;
    QByteArray band_ba = band_rec.toLocal8Bit();
    band_ba += "\n";
    file_out.write(band_ba, band_ba.length());
    file_out.close();
    return;
}

//绑定目录
void MainWindow::WriteBandLog(const QString &local_dir, const QString &remote_dir)
{
    QString write_str = CStr2LocalQStr("设置绑定目录[userid:") + QString::number(m_userid) + "] ";
    QString band_rec = local_dir + "|" + remote_dir;
    write_str += band_rec;
    WriteConnectLog(write_str);
    WriteConnectRec(local_dir, remote_dir);
}

void MainWindow::connectServer()
{
    //已连接
    if(isConnected())
        return;

    //获取ip
    int ip_n = 4;
    QSpinBox *pSpinIP[4] = {ui->spinIP1, ui->spinIP2, ui->spinIP3, ui->spinIP4};
    QString ipaddr;
    for(int i = 0; i < ip_n; i++){
        ipaddr += QString::number(pSpinIP[i]->value());
        if(i < ip_n - 1)
            ipaddr += ".";
    }
    //获取port
    int port = ui->spinPort->value();

    qDebug() <<"ip: "<< ipaddr;
    qDebug() <<"port: "<< port;

    m_server_sock->connectToHost(ipaddr, port);
    setConnectStatus(CONN_ING);     //连接中
    if(m_server_sock->waitForConnected()){
        setConnectStatus(CONN_OK);  //已连接
        WriteConnectLog("连接成功");
        //MyMessageBox::information("提示", "连接成功！");
    }
    else {
        setConnectStatus(CONN_NO);  //连接失败
        WriteConnectLog("连接失败");
        //MyMessageBox::critical("错误", "连接失败！");
    }

}

void MainWindow::disconnectServer()
{
    m_server_sock->disconnectFromHost();
    //m_server_sock->close();
    qDebug() << CStr2LocalQStr("断开连接！");
    WriteConnectLog("断开连接");
    //MyMessageBox::information("提示", "断开连接！");
    setConnectStatus(CONN_NO);      //未连接
}

//发送文件数据
void MainWindow::sendFileData(const QByteArray &json_ba, const QByteArray &content_ba)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(json_ba.length() + content_ba.length() + 1);
    //qDebug() << "pack len:" << len;
    QByteArray str_ba;
    //长度
    str_ba += (uchar)(0x00ff & len);
    str_ba += (uchar)((0xff00 & len) >> 8);
    //内容
    str_ba += json_ba;
    str_ba += "+";     //防止文件中有注释被json解析，用一个字符隔开
    //qDebug() << str;

    //QByteArray ba = str.toLatin1();
    str_ba += content_ba;
#ifdef DEBUG_COUT
    qDebug() <<"ba len: "<< ba.length();
    QString msg_str = "ba_len :" + QString::number(ba.length())
            + ", json_len:" + QString::number(json_str.length())
            + ", content_len:" + QString::number(content_ba.length());
    //QMessageBox::critical(nullptr, "!!!", msg_str);
#endif
    m_server_sock->write(str_ba);
    //m_server_sock->write(content_ba);
#ifdef DEBUG_COUT
    for(int i = 0; i < content_ba.length(); i++){
        std::cout << int((unsigned char)content_ba[i]) <<" ";
    }
    std::cout << endl;

    qDebug() <<"json len :" << json_str.length() <<"latin len:" << content_ba.length();
#endif
#if 0
    Json::CharReaderBuilder reader;
    Json::Value recvJson;
    JSONCPP_STRING errs;
    std::stringstream ss(content.toStdString());
    bool res = Json::parseFromStream(reader, ss, &recvJson, &errs);
    int std_len = recvJson["content"].asString().length();
    QString msg = "len :" + QString::number(std_len);
    for(int i = 0; i < std_len; i++){
        std::cout << int(recvJson["content"].asString()[i]) <<" ";
    }
    QMessageBox::critical(nullptr, "title", msg);
#endif
}

//发送数据（读文本框）
void MainWindow::sendDataFromBox()
{
    QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QByteArray str_ba;
    //长度
    str_ba += (uchar)(0x00ff & len);
    str_ba += (uchar)((0xff00 & len) >> 8);
    //内容
    str_ba += content;
    //qDebug() << str;
    m_server_sock->write(str_ba);
}

//发送数据
void MainWindow::sendData(const QByteArray &content_ba)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content_ba.length());
    QByteArray str_ba;
    //长度
    str_ba += (uchar)(0x00ff & len);
    str_ba += (uchar)((0xff00 & len) >> 8);
    //内容
    str_ba += content_ba;
    //qDebug() << str;
    m_server_sock->write(str_ba);

    //登出，断开连接
    if(m_recv_status == STAT_LOGOUT){
        disconnectServer();
    }
}

//发送数据(QString)
#if 0
void MainWindow::sendData(const QString &content)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QString str;
    //长度
    str += (uchar)(0x00ff & len);
    str += (uchar)((0xff00 & len) >> 8);
    //内容
    str += content;
    //qDebug() << str;
    m_server_sock->write(str.toLatin1());
}
#endif

//解析从服务端收到的消息
void MainWindow::parseJson(const QByteArray &str_ba)
{
    Json::CharReaderBuilder reader;
    Json::Value recvJson;
    JSONCPP_STRING errs;
    std::stringstream ss(str_ba.toStdString());
    bool res = Json::parseFromStream(reader, ss, &recvJson, &errs);
    if (!res || !errs.empty()) {
        qDebug() << "recv error!";
        return;
    }

    qDebug() << "function:" << recvJson["function"].asCString();

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
    else if(recvJson["function"] ==  "rmfileok"){
        parseJsonRmfileOK(recvJson);
    }
    else if(recvJson["function"] ==  "mkdirok"){
        parseJsonMkdirOK(recvJson);
    }
    //rmdirok已经和rmfileok合并
    else if(recvJson["function"] ==  "rmdirok"){
        parseJsonRmdirOK(recvJson);
    }
    else if(recvJson["function"] ==  "askallpath"){
        parseJsonAskAllPath(recvJson);
    }
    else if(recvJson["function"] ==  "downfile"){
        parseJsonDownfile(recvJson);
    }
    else if(recvJson["function"] ==  "downfileseg"){
        parseJsonDownfileseg(recvJson, str_ba);
    }
    else if(recvJson["function"] == "syncfile"){
        parseJsonSyncfile(recvJson);
    }
    else if(recvJson["function"] == "rmfile"){
        parseJsonRmfile(recvJson);
    }
    else if(recvJson["function"] == "mkdir"){
        parseJsonMkdir(recvJson);
    }
    return;
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
    if(1 == status){    //登录成功
        m_userid = recvJson["userid"].asInt();
        setUsername();
        setLoginUI();   //设置登录界面
        hideSubWin();   //隐藏子窗口
        MyMessageBox::information("提示", "登录成功！");

        //清空请求队列
        m_pFolder->SyncQClear();
    }
    else {
        if(0 == status)
            MyMessageBox::critical("错误", "登录失败！用户不存在！");
        else {
            MyMessageBox::critical("错误", "登录失败！密码错误！");
        }
        clearUser();
    }
    m_recv_status = STAT_WAIT;    //清标志位
}

//上传文件
void MainWindow::parseJsonUpfile(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "fileid:" << recvJson["fileid"].asInt();
    qDebug() << "startbit:" << recvJson["startbit"].asInt();
    qDebug() << "status: "<< recvJson["status"].asInt();
    qDebug() << "1.file_path: "<< m_filepath;

    int status = recvJson["status"].asInt();
    if(0 == status){    //已同步，无需上传
        qDebug() << m_filepath << CStr2LocalQStr("已同步，无需上传！");
        m_pFolder->WriteSyncLog("秒传");

        //MyMessageBox::information("提示", "已同步，无需上传！");
        int base_len = m_pFolder->getRootDir().length() + 1;
        QString save_path = m_filepath.mid(base_len);
        QString upfile_str = save_path + CStr2LocalQStr(" *文件上传完成！");
        ui->progStatus->setValue(100);
        ui->lnStatus->setText(upfile_str);
        QFileInfo file_info(m_filepath);
        int total_len = file_info.size();
        QString prog_str = getByteNumRatio(total_len, total_len);
        ui->lnBytes->setText(prog_str);
        qDebug() <<"parse upfile clear!!!";
        clearUpfile();
        /*
        if(m_filepath == m_pFolder->m_last_path){
            MyMessageBox::information("提示", "同步完成！");
        }*/
        m_pFolder->SyncQDequeue();      //出队

        sendDataSyncfile(save_path);
        return;
    }
    else {
        m_fileid = recvJson["fileid"].asInt();
        m_startbit = recvJson["startbit"].asInt();
        QString out_str = CStr2LocalQStr("真实上传，从") + QString::number(m_startbit)
                + CStr2LocalQStr("字节开始");
        m_pFolder->WriteSyncLog(out_str.toLocal8Bit());
    }
    qDebug() << "2.file_path: "<< m_filepath;

    m_recv_status = STAT_WAIT;    //清标志位

    int base_len = m_pFolder->getRootDir().length() + 1;
    QString upfile_str = m_filepath.mid(base_len) + CStr2LocalQStr(" *文件上传中......");
    ui->lnStatus->setText(upfile_str);
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
        qDebug() <<"parse upfileseg segwrong clear!!!";
        clearUpfile();

        m_pFolder->SyncQDequeue();      //处理完成，出队
        return;
    }
    else if(finish) {        //上传完成
        QFileInfo file_info(m_filepath);
        int total_len = file_info.size();
        int base_len = m_pFolder->getRootDir().length() + 1;
        QString save_path = m_filepath.mid(base_len);
        QString status_str = save_path + CStr2LocalQStr(" *文件上传完成！");
        ui->lnStatus->setText(status_str);
        ui->progStatus->setValue(100);
        QString prog_str = getByteNumRatio(total_len, total_len);
        ui->lnBytes->setText(prog_str);
#ifdef DEBUG_MSGBOX
        MyMessageBox::information("提示", "上传完成！");
#endif
        qDebug() <<"parse upfileseg finish clear!!!";
        clearUpfile();
        /*
        if(m_filepath == m_pFolder->m_last_path){
            MyMessageBox::information("提示", "同步完成！");
        }*/

        m_pFolder->SyncQDequeue();      //处理完成，出队

        //发送同步请求
        sendDataSyncfile(save_path);

        return;
    }
    //## 发文件片段，不清标志位
    //m_recv_status = STAT_WAIT;
    //本段上传成功，传下一段
    upfileBySeg();
}

//请求目录
void MainWindow::parseJsonAskAllPath(const Json::Value &recvJson)
{
    /************************************************
    "function": "askallpath"
    "path"[i]: 所有目录, 下标从0开始 例：1/dir1/
    "file"[i]: 所有文件，下标从0开始 例：1/dir1/file1
    "md5"[i]: 所有文件的md5，下标从0开始，和file一一对应
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "path size:" << recvJson["path"].size();
    qDebug() << "file size:" << recvJson["file"].size();

    QString status_str = CStr2LocalQStr("目录获取完成！");
    ui->lnStatus->setText(status_str);
    ui->lnBytes->setText("");

    m_pFolder->InitRemoteTree(recvJson);
}

//同步请求
void MainWindow::parseJsonSyncfile(const Json::Value &recvJson)
{
    /************************************************
    "function": "syncfile"
    "path": "1/2/test.txt"
    "md5":
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "path:" << recvJson["path"].asCString();
    qDebug() << "file md5:" << recvJson["md5"].asCString();

    //接收文件
    QString file_path = QString::fromLocal8Bit(recvJson["path"].asCString());
    sendDataDownfile(file_path);
}

//删除文件
void MainWindow::parseJsonRmfile(const Json::Value &recvJson)
{
    /************************************************
    "function": "rmfile",
    "path"  : "1/2/text.txt"
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "path:" << recvJson["path"].asCString();

    QString rel_path = QString::fromLocal8Bit(recvJson["path"].asCString());
    QString abs_path = m_pFolder->getRootDir() + "/" + rel_path;
    qDebug() <<"abs file:" << abs_path;

    //删除文件
    QFile::remove(abs_path);
    return;
}

//创建目录
void MainWindow::parseJsonMkdir(const Json::Value &recvJson)
{
    /************************************************
    "function": "mkdir"
    "path": "1/2/"
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "path:" << recvJson["path"].asCString();

    QString rel_dir = QString::fromLocal8Bit(recvJson["path"].asCString());
    QString abs_dir = m_pFolder->getRootDir() + "/" + rel_dir;
    qDebug() <<"abs dir:" << abs_dir;

    //创建目录
    createDir(abs_dir);
    return;
}

//删除目录
void MainWindow::parseJsonRmdir(const Json::Value &recvJson)
{    /************************************************
    "function": "rmdir"
    "path": "1/2/"
    "md5":
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "path:" << recvJson["path"].asCString();

    QString rel_dir = QString::fromLocal8Bit(recvJson["path"].asCString());
    QString abs_dir = m_pFolder->getRootDir() + "/" + rel_dir;
    qDebug() <<"abs dir:" << abs_dir;

    //删除目录
    QDir temp_dir(abs_dir);
    temp_dir.removeRecursively();
    return;
}

//下载文件
void MainWindow::parseJsonDownfile(const Json::Value &recvJson)
{
    /************************************************
    "function": "downfile"
    "fileid":
    "length":
    "md5":
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "fileid:" << recvJson["fileid"].asInt();
    qDebug() << "length:" << recvJson["length"].asInt();
    qDebug() << "md5:" << recvJson["md5"].asCString();

    int parse_len = recvJson["length"].asInt();

    m_startbit = 0;
    QString abs_path = m_pFolder->getRootDir() + "/" + m_filepath;
    QFileInfo file_info(abs_path);
    qDebug() << "tmp path: " << abs_path;
    if(file_info.exists()){
        int file_size = file_info.size();
        if(parse_len > file_size)     //断点续载
            m_startbit = file_size;
    }
    m_total_len = recvJson["length"].asInt();   //文件长度
    m_fileid = recvJson["fileid"].asInt();      //文件id

    QString out_str = CStr2LocalQStr("真实下载，从") + QString::number(m_startbit)
            + CStr2LocalQStr("字节开始");
    m_pFolder->WriteSyncLog(out_str.toLocal8Bit());

    int realpath_len = getRealpathLen(m_filepath);
    QString real_path = m_filepath.mid(0, realpath_len);
    abs_path = m_pFolder->getRootDir() + "/" + real_path;
    m_pFolder->RemoveWatchPath(abs_path);      //取消监视，防止下载后被上传

    QString downfile_str = real_path + CStr2LocalQStr(" *文件下载中......");
    ui->lnStatus->setText(downfile_str);
    //绝对路径（加'/'）
    m_filepath = m_pFolder->getRootDir() + "/" + m_filepath;

    //创建文件
    QFile file_out(m_filepath);
    file_out.open(QIODevice::WriteOnly);
    file_out.close();

    //开始下载
    downfileNextSeg();
}

//下载文件片段
void MainWindow::parseJsonDownfileseg(const Json::Value &recvJson, const QByteArray &str_ba)
{   
    /************************************************
    "function": "downfileseg",
    "size" : ,
    "md5" : "xxx"
    [一个无意义字符][紧接着是一个buf的content]
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "size:" << recvJson["size"].asInt();
    qDebug() << "md5:" << recvJson["md5"].asCString();

    //本次下载的内容
    int size = recvJson["size"].asInt();
    int offset = recvJson.toStyledString().size() + 1;
    QByteArray content_ba = str_ba.mid(offset);
    qDebug() <<"downseg len: "<< content_ba.length();
    if(size != content_ba.length()){
        MyMessageBox::critical("错误", "下载片段长度错误！");
        return;
    }
    else if(size == 0){
        MyMessageBox::critical("错误", "下载长度为0！");
        return;
    }
    //## 需校验片段的md5
    if(m_startbit + size > m_total_len){       //下载出错
        MyMessageBox::critical("错误", "下载长度错误！");
        return;
    }

    //下载文件片段
    downfileSeg(content_ba);

    m_startbit += size;    //本段下载完成，更新起始位置
    if(m_startbit == m_total_len){        //下载完成
        QFileInfo file_info(m_filepath);
        int total_len = file_info.size();
        int base_len = m_pFolder->getRootDir().length() + 1;
        int realpath_len = getRealpathLen(m_filepath);
        QString status_str = m_filepath.mid(base_len, realpath_len)
                + CStr2LocalQStr(" *文件下载完成！");
        ui->lnStatus->setText(status_str);
        ui->progStatus->setValue(100);
        QString prog_str = getByteNumRatio(total_len, total_len);
        ui->lnBytes->setText(prog_str);

        renameFileWithoutTmp();
#if 0
        MyMessageBox::information("提示", "下载完成！");
#endif
        qDebug() <<"parse downfileseg finish clear!!!";
        clearUpfile();

        m_pFolder->SyncQDequeue();      //处理完成，出队
        return;
    }
    //本段下载成功，下载下一段
    downfileNextSeg();
}

//删除文件
void MainWindow::parseJsonRmfileOK(const Json::Value &recvJson)
{
    /*******************************
     "function": "rmfileok"
     ******************************/
    qDebug() << "function: "<< recvJson["function"].asCString();
    m_pFolder->SyncQDequeue();      //处理完成，出队
}

//建立目录
void MainWindow::parseJsonMkdirOK(const Json::Value &recvJson)
{
    /*******************************
     "function": "mkdirok"
     ******************************/
    qDebug() << "function: "<< recvJson["function"].asCString();
    m_pFolder->SyncQDequeue();      //处理完成，出队
}

//删除目录（废弃，已和删除文件合并）
void MainWindow::parseJsonRmdirOK(const Json::Value &recvJson)
{
    /*******************************
     "function": "rmdirok"
     ******************************/
    qDebug() << "function: "<< recvJson["function"].asCString();
    m_pFolder->SyncQDequeue();      //处理完成，出队
}

//接收数据（读文本框）
void MainWindow::recvDataFromBox()
{
    QByteArray str_ba = ui->txtRecv->toPlainText().toLocal8Bit();
    qDebug() <<"recvData len: "<< str_ba.length();

#ifdef DEBUG_UPSEG
    qDebug() << "recv len: "<< len;
    qDebug() << "read : "<< str;
    qDebug() << "len : "<< str.length();
#endif
    qDebug() <<"recvJson len: "<< str_ba.length();

    parseJson(str_ba);
}

//接收数据
void MainWindow::recvData()
{
    QByteArray str_ba = m_server_sock->readAll();
    qDebug() <<"recvData len: "<< str_ba.length();

    //长度
    unsigned short len, len0, len1;
    len0 = (unsigned short)(uchar)str_ba[0];
    len1 = (unsigned short)(uchar)str_ba[1];
    len = len0 + (len1 << 8);
    qDebug() <<"B0: "<< len0 <<", B1: " << len1 <<", len: "<< len;

    //循环读
    int cnt = 0;
    while(str_ba.length() - 2 < len){
        cnt++;
        if(cnt > 5){
            m_recv_status = STAT_ERROR;
            MyMessageBox::critical("错误", "接收数据失败！");
            return;
        }
        if(m_server_sock->waitForReadyRead()){
            QByteArray read_ba = m_server_sock->readAll();
            qDebug() << "one read len:" << read_ba.length();
            str_ba += read_ba;
        }
    }
    //内容
    str_ba = str_ba.mid(2, len);

#ifdef DEBUG_UPSEG
    qDebug() << str_ba;
    qDebug() << "recv len: "<< len;
    qDebug() << "read : "<< str;
    qDebug() << "len : "<< str.length();
#endif
    QString str = QString::fromLocal8Bit(str_ba);
    if(m_recv_status != STAT_UPSEG && m_recv_status != STAT_DOWNSEG)
        ui->txtRecv->setText(str);

    qDebug() <<"recvJson len: "<< str_ba.length() <<" "<< len;

    parseJson(str_ba);

}

//打开注册页面
void MainWindow::openRegisterPage()
{
    connectServer();    //连接服务器
    m_pRegister->show();
}

//打开登录页面
void MainWindow::openLoginPage()
{
    connectServer();    //连接服务器
    m_pLogin->show();
}

//打开目录页面
void MainWindow::openFolderPage()
{
    if(!isLoginUser())
        return;

    //从绑定记录文件获取路径
    QString file_path = getUrecPath();
    QFile file_in(file_path);
    if(!file_in.open(QFile::ReadOnly)){
        qDebug() << "绑定记录不存在！";
        file_in.open(QFile::WriteOnly);     //创建文件
        QString default_rec = "|";          //local|remote
        file_in.write(default_rec.toLocal8Bit(), default_rec.length());
        file_in.close();

        QString write_str = CStr2LocalQStr("创建绑定记录[userid:") + QString::number(m_userid) + "] " ;
        write_str += default_rec;
        WriteConnectLog(write_str);
    }
    else {
        QByteArray conn_ba = file_in.readLine();
        QString conn_str = QString::fromLocal8Bit(conn_ba);
        /****************************
         本地目录|网盘同步目录
         ****************************/
        QStringList split_dir = conn_str.split('|');
        QString local_dir, remote_dir;
        if(split_dir.length() == 2){
            local_dir = split_dir[0];
            remote_dir = split_dir[1];
        }
        QString write_str;
        write_str += CStr2LocalQStr("读取绑定记录[userid:") + QString::number(m_userid) + "] " ;
        write_str += local_dir + "|" + remote_dir;
        WriteConnectLog(write_str);

        if(local_dir != ""){
            m_pFolder->setBandStatus(true);
            m_pFolder->setLocalDir(local_dir);
            m_pFolder->InitFolderTree();        //更新同步目录
        }
        else
            m_pFolder->setBandStatus(false);
    }

    m_pFolder->setUserid(m_userid);
    m_pFolder->show();
    return;
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

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_REGISTER;
    sendData(sendba);     //发送数据
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

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_LOGIN;
    sendData(sendba);     //发送数据
}

//退出登录
void MainWindow::sendDataLogout()
{
    qDebug() <<"logout...";
    //用户未登录
    if(!isLoginUser())
        return;

    qDebug() <<"username: "<< m_username;
    qDebug() <<"userid: "<< m_userid;

    Json::Value sendJson;
    sendJson["function"] = "logout";
    sendJson["userid"] = m_userid;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_pFolder->clearTree();

    clearUser();        //清空用户信息
    setNotLoginUI();    //设置未登录界面
    hideSubWin();

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_LOGOUT;
    sendData(sendba);     //发送数据
}

//分段上传文件
void MainWindow::upfileBySeg()
{
    if(m_filepath.length() == 0){
        MyMessageBox::critical("错误", "未选择文件！");
        m_recv_status = STAT_WAIT;
        return;
    }
    QFileInfo file_info(m_filepath);
    qint64 total_len = file_info.size();
#ifdef DEBUG_UPSEG
    qDebug() << CStr2LocalQStr("文件长度：") + QString::number(total_len);
#endif
    const int buf_len = 4096;
    int one_send_len = buf_len;
    qint64 remain_len = total_len - m_startbit;

    static QTime qtime_0 = QTime::currentTime();
    if(remain_len > 0){
        if(one_send_len > remain_len)
            one_send_len = int(remain_len);
        sendDataUpfileseg(m_filepath, m_startbit, one_send_len);

        //m_start_bit += one_send_len;
        //remain_len -= one_send_len;
        /*
        qDebug() << CStr2LocalQStr("已发送") << QString::number(m_start_bit)
                 << CStr2LocalQStr("字节，剩余") << QString::number(remain_len)
                 << CStr2LocalQStr("字节");
        */
        QTime qtime_1 = QTime::currentTime();
        if(qtime_1.second() != qtime_0.second()){
            qtime_0 = qtime_1;
            qDebug() << CStr2LocalQStr("") << QString::number(m_startbit)
                     << CStr2LocalQStr("字节，剩余") << QString::number(remain_len)
                     << CStr2LocalQStr("字节");

            //## 进度条
            QString bcnt_str = getByteNumRatio(m_startbit, total_len);
            ui->lnBytes->setText(bcnt_str);
            int prog_val = 100*m_startbit/total_len;
            ui->progStatus->setValue(prog_val);
        }
        m_recv_status = STAT_UPSEG;
    }
    else {
        qDebug() <<"upfilebyseg clear!!!";
        clearUpfile();
        MyMessageBox::critical("错误", "本次上传0字节！");
        m_recv_status = STAT_WAIT;
    }
}

//上传文件
void MainWindow::sendDataUpfile(const QString &file_path)
{
    qDebug() << "sendDataUpfile" << file_path;
    //用户未登录
    if(!isLoginUser())
        return;

    QString full_path = m_pFolder->getRootDir() + "/" + file_path;
    m_filepath = full_path;
    //QMessageBox::information(nullptr, "info", full_path);

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

    QByteArray sendba = QStr2LocalBa(sendbuf);

    m_recv_status = STAT_UPFILE;
    sendData(sendba);     //发送数据
}

//发送文件片段
void MainWindow::sendDataUpfileseg(const QString &file_path, qint64 start_bit, int len)
{
    qDebug() << "upfileseg";
    //用户未登录
    if(!isLoginUser())
        return;

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

    m_startbit += seg_ba.length();

    //QByteArray是字节流，不能直接转QString，否则'\0'之后都会被过滤
    //QTextCodec *codec = QTextCodec::codecForName("KOI8-R");

    //QString seg_content = codec->toUnicode(seg_ba);
    //QString seg_md5 = QStr2MD5(seg_content);

    std::string seg_content_std;
    seg_content_std.resize(len);
    for(int i = 0; i < len; i++){
        seg_content_std[i] = seg_ba[i];
    }
    QString seg_md5 = QBa2MD5(seg_ba);
#if 0
    qDebug() <<"***************************";
    qDebug() << "len="<<len <<", read_len:" << seg_ba.length();

    //int str_len = seg_content.toStdU32String().length();

    //QString msg = "len=" + QString::number(len) + ", read_len=" + QString::number(seg_content.length())
    //        + ", str_len=" + QString::number(str_len);
    //QMessageBox::information(nullptr, "info", msg);

    qDebug() <<"***************************";

    qDebug() <<"upfileseg...";
    qDebug() <<"fileid: "<< m_fileid;
    qDebug() <<"md5: "<< seg_md5;
    qDebug() <<"length:" << seg_content_std.length();
    //qDebug() <<"content: "<< seg_content;
#endif
    Json::Value sendJson;
    sendJson["function"] = "upfileseg";
    sendJson["fileid"] = m_fileid;
    sendJson["md5"] = seg_md5.toStdString();
    //sendJson["content"] = seg_content_std;
    sendJson["length"] = seg_content_std.length();

    //std::string recv_seg = sendJson["content"].asString();
    //qDebug() << "recv_seg len: "<< recv_seg.size();

    //### 用int数组存字符
#if 0
    for(int i = 0; i < len; i++){
        sendJson["content"][i] = int(seg_ba[i]);
    }
#endif
    QByteArray sendba = sendJson.toStyledString().data();
    //ui->txtSend->setText(sendbuf);

#if 0
    Json::CharReaderBuilder reader;
    Json::Value recvJson;
    JSONCPP_STRING errs;
    std::stringstream ss(sendbuf.toStdString());
    bool res = Json::parseFromStream(reader, ss, &recvJson, &errs);
    int std_len = recvJson["content"].asString().length();
    QString msg = "sbuf len :" + QString::number(sendJson.toStyledString().length())
            + ",data len:" + QString::number(sendbuf.length())
            + ",rbuf len:" + QString::number(recvJson.toStyledString().length())
            + ",send len:" + QString::number(sendJson["content"].asString().length())
            + ",recv len:" + QString::number(recvJson["content"].asString().length());
    for(int i = 0; i < std_len; i++){
        std::cout << int(recvJson["content"].asString()[i]) <<" ";
    }
    QMessageBox::warning(nullptr, "title", msg);
#endif

    sendFileData(sendba, seg_ba);     //发送数据
}

//请求目录
void MainWindow::sendDataAskAllPath()
{
    /********************************
    "function": "askallpath"
     *******************************/
    qDebug() << "askallpath";
    //用户未登录
    if(!isLoginUser())
        return;

    Json::Value sendJson;
    sendJson["function"] = "askallpath";

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_WAIT;
    sendData(sendba);     //发送数据

}

//下载文件
void MainWindow::sendDataDownfile(const QString &file_path)
{
    /********************************
    "function": "downfile",
    "path": "1/2/test.txt"
     *******************************/
    qDebug() << "downfile";
    //用户未登录
    if(!isLoginUser())
        return;

    Json::Value sendJson;
    sendJson["function"] = "downfile";
    sendJson["path"] = file_path.toStdString();

    m_filepath = file_path + ".tmp";
    qDebug() << "file_path: " << m_filepath;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_DOWNSEG;
    sendData(sendba);     //发送数据
}

//同步文件
void MainWindow::sendDataSyncfile(const QString &file_path)
{
    /********************************
    "function": "syncfile",
    "path"  : "1/2/test.txt",
    "md5" : "xxx"
     *******************************/
    qDebug() << "syncfile";
    //用户未登录
    if(!isLoginUser())
        return;

    QString abs_path = m_pFolder->getRootDir() + "/" + file_path;
    QString md5 = getFileMD5(abs_path);

    Json::Value sendJson;
    sendJson["function"] = "syncfile";
    sendJson["path"] = file_path.toStdString();
    sendJson["md5"] = md5.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_WAIT;
    sendData(sendba);     //发送数据

}

//下载文件片
void MainWindow::sendDataDownfileseg(qint64 startbit, int file_id, int len)
{
    /********************************
    "function": "downfileseg",
    "fileid": 52,
    "startbit": 0,
    "size": 4096
     *******************************/
    qDebug() << "downfileseg";
    //用户未登录
    if(!isLoginUser())
        return;

    Json::Value sendJson;
    sendJson["function"] = "downfileseg";
    sendJson["fileid"] = file_id;
    sendJson["startbit"] = startbit;
    sendJson["size"] = len;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_WAIT;
    sendData(sendba);     //发送数据
}

//下载文件片段
void MainWindow::downfileSeg(const QByteArray &content_ba)
{
    qint64 start_bit = m_startbit;
    QString file_path = m_filepath;
    int len = content_ba.length();

    qDebug() <<"down file seg......";
    //QMessageBox::information(nullptr, "path", file_path);

    //不存在则创建
    int last_pos = file_path.lastIndexOf('/');
    QString base_path = file_path.mid(0, last_pos);
    QDir base_dir(base_path);
    if(!base_dir.exists()) {
        qDebug() <<"base path:" << base_path;
        if(!createFile(file_path)){
           qDebug() << "Create "<< file_path << " ERROR";
        }
    }
    fstream fout(file_path.toLocal8Bit(), ios::in | ios::out | ios::binary);
    fout.seekp(start_bit);
    fout.write(content_ba, len);
    fout.close();

#if 0
    QFile file_out(file_path);
    if(!file_out.open(QIODevice::ReadWrite)){
        qDebug() << CStr2LocalQStr("文件打开失败！") << file_path.toLocal8Bit();
        MyMessageBox::critical("提示", "文件打开失败！");
        return;
    }
    qDebug() <<"*****************************************"<< len;
    qDebug() <<"filepath:" << m_filepath << " startbit:" << start_bit <<"ba len:"<< len;

    qDebug() <<"content ba";
    qDebug() << content_ba;

    file_out.seek(file_out.size());    //移动文件指针
    file_out.write(content_ba, len);    //写入文件
    file_out.close();        //关闭文件
#endif
}

//分段下载文件
void MainWindow::downfileNextSeg()
{
    if(m_filepath.length() == 0){
        MyMessageBox::critical("错误", "未选择文件！");
        m_recv_status = STAT_WAIT;
        return;
    }

    const int buf_len = 4096;
    int one_recv_len = buf_len;

    qint64 total_len = m_total_len;
    qint64 remain_len = total_len - m_startbit;
    if(remain_len > 0){
        static QTime qtime_0 = QTime::currentTime();

        //下载文件片段
        if(remain_len < one_recv_len)
            one_recv_len = remain_len;
        sendDataDownfileseg(m_startbit, m_fileid, one_recv_len);

        QTime qtime_1 = QTime::currentTime();
        if(qtime_1.second() != qtime_0.second()){
            qtime_0 = qtime_1;
            qDebug() << CStr2LocalQStr("已下载") << QString::number(m_startbit)
                     << CStr2LocalQStr("字节，剩余") << QString::number(remain_len)
                     << CStr2LocalQStr("字节");

            //## 进度条
            QString bcnt_str = getByteNumRatio(m_startbit, total_len);
            ui->lnBytes->setText(bcnt_str);
            int prog_val = 100*m_startbit/total_len;
            ui->progStatus->setValue(prog_val);
        }
        m_recv_status = STAT_WAIT;
    }
    else {
        MyMessageBox::critical("错误", "下载字节数为0！");
        return;
    }
}

//删除文件
void MainWindow::sendDataRmfile(const QString &file_path)
{
    qDebug() << "rmfile "<< file_path;
    //用户未登录
    if(!isLoginUser())
        return;

    Json::Value sendJson;
    sendJson["function"] = "rmfile";
    sendJson["path"] = file_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);

    m_recv_status = STAT_RMFILE;
    sendData(sendba);     //发送数据
}

//创建目录
void MainWindow::sendDataMkdir(const QString &dir_path)
{
    qDebug() << "mkdir "<< dir_path.toLocal8Bit();
    //用户未登录
    if(!isLoginUser())
        return;

    Json::Value sendJson;
    sendJson["function"] = "mkdir";
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);

    //### 测试中文编码转回
#if DEBUG_CNCODE
    Json::CharReaderBuilder reader;
    Json::Value recvJson;
    JSONCPP_STRING errs;
    std::stringstream ss(sendba.toStdString());
    //std::stringstream ss(sendbuf.toLocal8Bit().toStdString());
    bool res = Json::parseFromStream(reader, ss, &recvJson, &errs);
    if (!res || !errs.empty()) {
        qDebug() << "recv error!";
        return;
    }

    QFile file_in("output-gbk.txt");
    file_in.open(QFile::WriteOnly);
    //int str_len = dir_path.toLocal8Bit().toStdString().length();
    //file_in.write(dir_path.toLocal8Bit().toStdString().c_str(), str_len);
    int str_len = recvJson["path"].asString().length();
    file_in.write(recvJson["path"].asString().c_str(), str_len);
    file_in.close();
#endif
    m_recv_status = STAT_MKDIR;
    sendData(sendba);     //发送数据
}

//删除目录
void MainWindow::sendDataRmdir(const QString &dir_path)
{
    qDebug() << "rmdir "<< dir_path;
    //用户未登录
    if(!isLoginUser())
        return;

    Json::Value sendJson;
    //sendJson["function"] = "rmdir";
    sendJson["function"] = "rmfile";    //删文件和删目录命令合并
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_RMDIR;
    sendData(sendba);     //发送数据
}

//窗口关闭
void MainWindow::closeEvent(QCloseEvent *event)
{
    //若用户为登录状态，发送退出信号
    if(m_userid >= 0){
        sendDataLogout();
    }
    else {
        //sendDataLogout();
        qDebug() << CStr2LocalQStr("用户已退出");
    }
    event->accept();
    qDebug() << CStr2LocalQStr("主窗口关闭！");
}
