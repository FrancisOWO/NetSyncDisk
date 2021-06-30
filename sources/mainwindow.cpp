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

    //���Ӵ���
    connect(ui->pbtnRegister, SIGNAL(clicked()), this, SLOT(openRegisterPage()));
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(openLoginPage()));
    connect(ui->pbtnFolder, SIGNAL(clicked()), this, SLOT(openFolderPage()));

    //��������
    connect(m_pRegister, SIGNAL(completed()), this, SLOT(sendDataRegister()));
    connect(m_pLogin, SIGNAL(completed()), this, SLOT(sendDataLogin()));
    connect(ui->pbtnLogout, SIGNAL(clicked()), this, SLOT(sendDataLogout()));

    //�ϴ��ļ�
    connect(m_pFolder, SIGNAL(upfile(const QString &)), this, SLOT(sendDataUpfile(const QString &)));
    //ɾ���ļ�
    connect(m_pFolder, SIGNAL(rmfile(const QString &)), this, SLOT(sendDataRmfile(const QString &)));
    //����Ŀ¼
    connect(m_pFolder, SIGNAL(mkdir(const QString &)), this, SLOT(sendDataMkdir(const QString &)));
    //ɾ��Ŀ¼
    connect(m_pFolder, SIGNAL(rmdir(const QString &)), this, SLOT(sendDataRmdir(const QString &)));
    //�ֶδ��ļ�
    connect(ui->pbtnUpfile, SIGNAL(clicked()), this, SLOT(upfileBySeg()));


    connect(m_server_sock, &QTcpSocket::connected, [=](){
        MyMessageBox::information("��ʾ", "���ӳɹ���");
        m_is_connected = 1;
    });
    connect(m_server_sock, &QTcpSocket::readyRead, [=](){
#if 0
        MyMessageBox::information("��ʾ", "���Զ�ȡ���ݣ�");
#endif
        recvData();     //��������
    });
    typedef void (QAbstractSocket::*QAbstractSocketErrorSignal)(QAbstractSocket::SocketError);
    connect(m_server_sock, static_cast<QAbstractSocketErrorSignal>(&QTcpSocket::error), [=](){
        if(1 == m_is_connected)
            MyMessageBox::information("��ʾ", "���ӶϿ���");
        else
            MyMessageBox::information("��ʾ", "����ʧ�ܣ�");
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
    ui->lnUsername->setText(CStr2LocalQStr("δ��¼"));
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

//��������
void MainWindow::sendData(const QString &content)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QString str;
    //����
    str += (uchar)(0x00ff & len);
    str += (uchar)((0xff00 & len) >> 8);
#ifdef DEBUG_UPSEG
    qDebug() <<"slen :" << str.length();
#endif
    //����
    str += content;
    //qDebug() << str;
    //qDebug() <<"len :" << str.length();
    m_server_sock->write(str.toLatin1());

}

//�����ӷ�����յ�����Ϣ
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
    //����function���н���
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

//ע��
void MainWindow::parseJsonRegister(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();

    int status = recvJson["status"].asInt();
    if(1 == status){
        MyMessageBox::information("��ʾ", "ע��ɹ���");
    }
    else {
        MyMessageBox::critical("����", "ע��ʧ�ܣ����û����ѱ�ע�ᣡ");
    }
    m_recv_status = STAT_WAIT;    //���־λ
}

//��¼
void MainWindow::parseJsonLogin(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "userid:" << recvJson["userid"].asInt();

    int status = recvJson["status"].asInt();
    if(1 == status){
        MyMessageBox::information("��ʾ", "��¼�ɹ���");
        m_userid = recvJson["userid"].asInt();
        setUsername();
    }
    else {
        MyMessageBox::critical("����", "��¼ʧ�ܣ��û��������벻��ȷ��");
        clearUserid();
        clearUsername();
    }
    m_recv_status = STAT_WAIT;    //���־λ
}

//�ϴ��ļ�
void MainWindow::parseJsonUpfile(const Json::Value &recvJson)
{
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "fileid:" << recvJson["fileid"].asInt();
    qDebug() << "startbit:" << recvJson["startbit"].asInt();

    int status = recvJson["status"].asInt();
    if(0 == status){    //��ͬ���������ϴ�
        MyMessageBox::information("��ʾ", "��ͬ���������ϴ���");
        clearUpfile();
        return;
    }
    m_fileid = recvJson["fileid"].asInt();
    m_start_bit = recvJson["startbit"].asInt();
    m_recv_status = STAT_WAIT;    //���־λ
    //��ʼ�ϴ�
    upfileBySeg();
}

//�ϴ��ļ�Ƭ��
void MainWindow::parseJsonUpfileseg(const Json::Value &recvJson)
{
#ifdef DEBUG_UPSEG
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "finish:" << recvJson["finish"].asInt();
#endif
    int segtrue = recvJson["segtrue"].asInt();
    int finish = recvJson["finish"].asInt();

    if(!segtrue){       //�����ϴ�ʧ��
        MyMessageBox::critical("��ʾ", "�������");
        clearUpfile();
        return;
    }
    else if(finish) {        //�ϴ����
        MyMessageBox::information("��ʾ", "�ϴ���ɣ�");
        clearUpfile();
        return;
    }
    //## ���ļ�Ƭ�Σ������־λ
    //m_recv_status = STAT_WAIT;
    //�����ϴ��ɹ�������һ��
    upfileBySeg();
}

//ɾ���ļ�
void MainWindow::parseJsonRmfile(const Json::Value &recvJson)
{

}

//����Ŀ¼
void MainWindow::parseJsonMkdir(const Json::Value &recvJson)
{

}

//ɾ��Ŀ¼
void MainWindow::parseJsonRmdir(const Json::Value &recvJson)
{

}

//��������
void MainWindow::recvData()
{
    QByteArray str = m_server_sock->readAll();
    //����
    unsigned short len, len0, len1;
    len0 = (unsigned short)str[0];
    len1 = (unsigned short)str[1];
    len = len0 + (len1 << 8);
    //����
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

//��ע��ҳ��s
void MainWindow::openRegisterPage()
{
    m_pRegister->show();
}

//�򿪵�¼ҳ��
void MainWindow::openLoginPage()
{
    m_pLogin->show();
}

//��Ŀ¼ҳ��
void MainWindow::openFolderPage()
{
    QString root_dir = "E:/test";
    m_pFolder->setRootDir(root_dir);
    m_pFolder->InitFolderTree();
    m_pFolder->show();
}

//ע��
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

    //pRegister->clearData();   //�����Ϣ
    //m_pRegister->close();     //�رմ���

    m_recv_status = STAT_REGISTER;
    sendData(sendbuf);     //��������
}

//��¼
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

    //pLogin->clearData();  //�����Ϣ
    //m_pLogin->close();    //�رմ���
    m_username = usname;

    m_recv_status = STAT_LOGIN;
    sendData(sendbuf);     //��������
}

//�˳���¼
void MainWindow::sendDataLogout()
{
    //δ��¼
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

    clearUserid();    //�÷Ƿ�id

    m_recv_status = STAT_WAIT;
    sendData(sendbuf);     //��������
}

//�ֶ��ϴ��ļ�
void MainWindow::upfileBySeg()
{
    if(m_file_path.length() == 0){
        MyMessageBox::critical("����", "δѡ���ļ���");
        m_recv_status = STAT_WAIT;
        return;
    }
    QFileInfo file_info(m_file_path);
    qint64 total_len = file_info.size();
#ifdef DEBUG_UPSEG
    qDebug() << CStr2LocalQStr("�ļ����ȣ�") + QString::number(total_len);
#endif
    const int buf_len = 4096;
    int one_send_len = buf_len;
    qint64 remain_len = total_len - m_start_bit;

    static QTime qtime_0 = QTime::currentTime();
    QTime qtime_1 = QTime::currentTime();
    if(qtime_1.second() != qtime_0.second()){
        qtime_0 = qtime_1;
        qDebug() << CStr2LocalQStr("") << QString::number(m_start_bit)
                 << CStr2LocalQStr("�ֽڣ�ʣ��") << QString::number(remain_len)
                 << CStr2LocalQStr("�ֽ�");
    }

    if(remain_len > 0){
        if(one_send_len > remain_len)
            one_send_len = int(remain_len);
        sendDataUpfileseg(m_file_path, m_start_bit, one_send_len);

        //m_start_bit += one_send_len;
        //remain_len -= one_send_len;
        /*
        qDebug() << CStr2LocalQStr("�ѷ���") << QString::number(m_start_bit)
                 << CStr2LocalQStr("�ֽڣ�ʣ��") << QString::number(remain_len)
                 << CStr2LocalQStr("�ֽ�");
        */
    m_recv_status = STAT_UPSEG;
    }
    else {
        clearUpfile();
        m_recv_status = STAT_WAIT;
    }
}

//�ϴ��ļ�
void MainWindow::sendDataUpfile(const QString &file_path)
{
    QString full_path = m_pFolder->getRootDir() + "/" + file_path;
    m_file_path = full_path;

    QFileInfo file_info(full_path);
    quint64 file_size = file_info.size();
    if(file_size <= 0){
        MyMessageBox::critical("����", "�����ϴ����ļ���");
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
    sendData(sendbuf);     //��������
}

//�����ļ�Ƭ��
void MainWindow::sendDataUpfileseg(const QString &file_path, qint64 start_bit, int len)
{
    QFile file_in(file_path);
    if(!file_in.open(QIODevice::ReadOnly)){
        qDebug() << CStr2LocalQStr("�ļ���ʧ�ܣ�");
        return;
    }
    if(start_bit >= file_in.size()){
        qDebug() << CStr2LocalQStr("����λ�ô���") + "start_bit=" + QString::number(start_bit);
        return;
    }

    file_in.seek(start_bit);    //�ƶ��ļ�ָ�룬
    QByteArray seg_ba = file_in.read(len);     //��ȡ�ļ�
    file_in.close();        //�ر��ļ�

    m_start_bit += seg_ba.length();

    //QByteArray���ֽ���������ֱ��תQString������'\0'֮�󶼻ᱻ����
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
    //### ��int������ַ�
    for(int i = 0; i < len; i++){
        sendJson["content"][i] = int(seg_ba[i]);
    }
    QString sendbuf = sendJson.toStyledString().data();
    //ui->txtSend->setText(sendbuf);

    sendData(sendbuf);     //��������
}

//ɾ���ļ�
void MainWindow::sendDataRmfile(const QString &file_path)
{
    Json::Value sendJson;
    sendJson["function"] = "rmfile";
    sendJson["path"] = file_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_RMFILE;
    sendData(sendbuf);     //��������
}

//����Ŀ¼
void MainWindow::sendDataMkdir(const QString &dir_path)
{
    Json::Value sendJson;
    sendJson["function"] = "mkdir";
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_MKDIR;
    sendData(sendbuf);     //��������
}

//ɾ��Ŀ¼
void MainWindow::sendDataRmdir(const QString &dir_path)
{
    Json::Value sendJson;
    //sendJson["function"] = "rmdir";
    sendJson["function"] = "rmfile";    //ɾ�ļ���ɾĿ¼����ϲ�
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    m_recv_status = STAT_RMDIR;
    sendData(sendbuf);     //��������
}

//���ڹر�
void MainWindow::closeEvent(QCloseEvent *event)
{
    //���û�Ϊ��¼״̬�������˳��ź�
    if(m_userid >= 0){
        sendDataLogout();
    }
    else {
        sendDataLogout();
        qDebug() << CStr2LocalQStr("�û����˳�");
    }
    event->accept();
    qDebug() << CStr2LocalQStr("�����ڹرգ�");
}
