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
#include <string>

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
    clearUsername();
    clearUpfile();
    setConStatus(CONN_NO);

    m_pRegister = new FormRegister;
    m_pLogin = new FormLogin;
    m_pFolder = new FormFolder;

    //ui->spinIP1->setValue(192);
    //ui->spinIP2->setValue(168);
    //ui->spinIP3->setValue(43);
    //ui->spinIP4->setValue(230);
    ui->progStatus->setValue(0);

    ui->spinIP1->setValue(10);
    ui->spinIP2->setValue(60);
    ui->spinIP3->setValue(102);
    ui->spinIP4->setValue(252);

    ui->spinPort->setValue(20230);

    InitSocket();

    //�Զ����ӷ�����
    connectServer();
}

void MainWindow::InitConnections()
{
    connect(ui->pbtnConnect, SIGNAL(clicked()), this, SLOT(connectServer()));
    connect(ui->pbtnDisconnect, SIGNAL(clicked()), this, SLOT(disconnectServer()));
    connect(ui->pbtnSend, SIGNAL(clicked()), this, SLOT(sendDataFromBox()));
    connect(ui->pbtnRecv, SIGNAL(clicked()), this, SLOT(recvDataFromBox()));

    //���Ӵ���
    connect(ui->pbtnRegister, SIGNAL(clicked()), this, SLOT(openRegisterPage()));
    connect(ui->pbtnLogin, SIGNAL(clicked()), this, SLOT(openLoginPage()));
    connect(ui->pbtnFolder, SIGNAL(clicked()), this, SLOT(openFolderPage()));

    //��������
    //ע��
    connect(m_pRegister, SIGNAL(completed()), this, SLOT(sendDataRegister()));
    //��¼
    connect(m_pLogin, SIGNAL(completed()), this, SLOT(sendDataLogin()));
    //�ǳ�
    connect(ui->pbtnLogout, SIGNAL(clicked()), this, SLOT(sendDataLogout()));
    //����Ŀ¼
    connect(ui->pbtnAskPath, SIGNAL(clicked()), this, SLOT(sendDataAskAllPath()));

    //�ϴ��ļ�
    connect(m_pFolder, SIGNAL(upfile(const QString &)), this, SLOT(sendDataUpfile(const QString &)));
    //ɾ���ļ�
    connect(m_pFolder, SIGNAL(rmfile(const QString &)), this, SLOT(sendDataRmfile(const QString &)));
    //����Ŀ¼
    connect(m_pFolder, SIGNAL(mkdir(const QString &)), this, SLOT(sendDataMkdir(const QString &)));
    //ɾ��Ŀ¼
    connect(m_pFolder, SIGNAL(rmdir(const QString &)), this, SLOT(sendDataRmdir(const QString &)));
    //�ֶδ��ļ�
    //connect(ui->pbtnUpfile, SIGNAL(clicked()), this, SLOT(upfileBySeg()));
    //�����ļ�
    connect(m_pFolder, SIGNAL(downfile(const QString &)), this, SLOT(sendDataDownfile(const QString &)));


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
            MyMessageBox::critical("����", "����ʧ�ܣ�");
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
    m_startbit = 0;

    qDebug() <<"!!! clear path:" << m_filepath;
    m_filepath.clear();
}

void MainWindow::InitSocket()
{
    m_server_sock = new QTcpSocket(this);
    m_server_sock->setReadBufferSize(65536);
}

//�ж��û��Ƿ��¼
bool MainWindow::isLoginUser()
{
    bool is_login = (m_userid >= 0);
    if(!is_login){
        qDebug() << "ERROR: Not Login!";
        //MyMessageBox::critical("����", "�û�δ��¼��");
    }
    is_login = 1;
    return is_login;
}

void MainWindow::setConStatus(int status)
{
    QString str;
    if(status == CONN_NO){
        str = CStr2LocalQStr("δ����");
    }
    else if(status == CONN_ING){
        str = CStr2LocalQStr("������......");
    }
    else if(status == CONN_OK){
        str = CStr2LocalQStr("������");
    }
    ui->lnConStatus->setText(str);
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
    setConStatus(CONN_ING);
    if(m_server_sock->waitForConnected()){
        setConStatus(CONN_OK);
        //MyMessageBox::information("��ʾ", "���ӳɹ���");
    }
    else {
        setConStatus(CONN_NO);
        //MyMessageBox::critical("����", "����ʧ�ܣ�");
    }

}

void MainWindow::disconnectServer()
{
    m_server_sock->disconnect();
    //m_server_sock->close();
    MyMessageBox::information("��ʾ", "�Ͽ����ӣ�");
    ui->lnConStatus->setText(CStr2LocalQStr("δ����"));
}

//�����ļ�����
void MainWindow::sendFileData(const QByteArray &json_ba, const QByteArray &content_ba)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(json_ba.length() + content_ba.length() + 1);
    //qDebug() << "pack len:" << len;
    QByteArray str_ba;
    //����
    str_ba += (uchar)(0x00ff & len);
    str_ba += (uchar)((0xff00 & len) >> 8);
    //����
    str_ba += json_ba;
    str_ba += "+";     //��ֹ�ļ�����ע�ͱ�json��������һ���ַ�����
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

//�������ݣ����ı���
void MainWindow::sendDataFromBox()
{
    QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QByteArray str_ba;
    //����
    str_ba += (uchar)(0x00ff & len);
    str_ba += (uchar)((0xff00 & len) >> 8);
    //����
    str_ba += content;
    //qDebug() << str;
    m_server_sock->write(str_ba);
}

//��������
void MainWindow::sendData(const QByteArray &content_ba)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content_ba.length());
    QByteArray str_ba;
    //����
    str_ba += (uchar)(0x00ff & len);
    str_ba += (uchar)((0xff00 & len) >> 8);
    //����
    str_ba += content_ba;
    //qDebug() << str;
    m_server_sock->write(str_ba);
}

//��������(QString)
#if 0
void MainWindow::sendData(const QString &content)
{
    //QString content = ui->txtSend->toPlainText();
    unsigned short len = (unsigned short)(content.length());
    QString str;
    //����
    str += (uchar)(0x00ff & len);
    str += (uchar)((0xff00 & len) >> 8);
    //����
    str += content;
    //qDebug() << str;
    m_server_sock->write(str.toLatin1());
}
#endif

//�����ӷ�����յ�����Ϣ
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
    else if(recvJson["function"] ==  "rmfileok"){
        parseJsonRmfileOK(recvJson);
    }
    else if(recvJson["function"] ==  "mkdirok"){
        parseJsonMkdirOK(recvJson);
    }
    //rmdirok�Ѿ���rmfileok�ϲ�
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

        //����������
        m_pFolder->SyncQClear();
    }
    else {
        if(0 == status)
            MyMessageBox::critical("����", "��¼ʧ�ܣ��û������ڣ�");
        else {
            MyMessageBox::critical("����", "��¼ʧ�ܣ��������");
        }
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
    qDebug() << "status: "<< recvJson["status"].asInt();
    qDebug() << "1.file_path: "<< m_filepath;

    int status = recvJson["status"].asInt();
    if(0 == status){    //��ͬ���������ϴ�
        qDebug() << m_filepath << CStr2LocalQStr("��ͬ���������ϴ���");
        //MyMessageBox::information("��ʾ", "��ͬ���������ϴ���");
        int base_len = m_pFolder->getRootDir().length() + 1;
        QString upfile_str = m_filepath.mid(base_len) + CStr2LocalQStr(" *�ļ��ϴ���ɣ�");
        ui->progStatus->setValue(100);
        ui->lnStatus->setText(upfile_str);
        qDebug() <<"parse upfile clear!!!";
        clearUpfile();

        if(m_filepath == m_pFolder->m_last_path){
            MyMessageBox::information("��ʾ", "ͬ����ɣ�");
        }

        m_pFolder->SyncQDequeue();      //����
        return;
    }
    m_fileid = recvJson["fileid"].asInt();
    m_startbit = recvJson["startbit"].asInt();

    qDebug() << "2.file_path: "<< m_filepath;

    m_recv_status = STAT_WAIT;    //���־λ

    int base_len = m_pFolder->getRootDir().length() + 1;
    QString upfile_str = m_filepath.mid(base_len) + CStr2LocalQStr(" *�ļ��ϴ���......");
    ui->lnStatus->setText(upfile_str);
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
        qDebug() <<"parse upfileseg segwrong clear!!!";
        clearUpfile();

        m_pFolder->SyncQDequeue();      //������ɣ�����
        return;
    }
    else if(finish) {        //�ϴ����
        QFileInfo file_info(m_filepath);
        int total_len = file_info.size();
        int base_len = m_pFolder->getRootDir().length() + 1;
        QString status_str = m_filepath.mid(base_len)
                + CStr2LocalQStr(" *�ļ��ϴ���ɣ�");
        ui->lnStatus->setText(status_str);
        ui->progStatus->setValue(100);
        QString prog_str = getByteNumRatio(total_len, total_len);
#ifdef DEBUG_MSGBOX
        MyMessageBox::information("��ʾ", "�ϴ���ɣ�");
#endif
        qDebug() <<"parse upfileseg finish clear!!!";
        clearUpfile();

        if(m_filepath == m_pFolder->m_last_path){
            MyMessageBox::information("��ʾ", "ͬ����ɣ�");
        }

        m_pFolder->SyncQDequeue();      //������ɣ�����
        return;
    }
    //## ���ļ�Ƭ�Σ������־λ
    //m_recv_status = STAT_WAIT;
    //�����ϴ��ɹ�������һ��
    upfileBySeg();
}

//����Ŀ¼
void MainWindow::parseJsonAskAllPath(const Json::Value &recvJson)
{
    /************************************************
    "function": "askallpath"
    "path"[i]: ����Ŀ¼, �±��0��ʼ ����1/dir1/
    "file"[i]: �����ļ����±��0��ʼ ����1/dir1/file1
    "md5"[i]: �����ļ���md5���±��0��ʼ����fileһһ��Ӧ
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "path size:" << recvJson["path"].size();
    qDebug() << "file size:" << recvJson["file"].size();

    QString status_str = CStr2LocalQStr("Ŀ¼��ȡ��ɣ�");
    ui->lnStatus->setText(status_str);

    m_pFolder->InitRemoteTree(recvJson);
}

//�����ļ�
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

    m_startbit = 0;
    m_total_len = recvJson["length"].asInt();   //�ļ�����
    m_fileid = recvJson["fileid"].asInt();      //�ļ�id

    QString downfile_str = m_filepath + CStr2LocalQStr(" *�ļ�������......");
    ui->lnStatus->setText(downfile_str);
    //����·������'/'��
    m_filepath = m_pFolder->getRootDir() + "/" + m_filepath;

    //�����ļ�
    QFile file_out(m_filepath);
    file_out.open(QIODevice::WriteOnly);
    file_out.close();

    //��ʼ����
    downfileNextSeg();
}

//�����ļ�Ƭ��
void MainWindow::parseJsonDownfileseg(const Json::Value &recvJson, const QByteArray &str_ba)
{
    /************************************************
    "function": "downfileseg",
    "size" : ,
    "md5" : "xxx"
    [һ���������ַ�][��������һ��buf��content]
     ************************************************/
    qDebug() << "function:" << recvJson["function"].asCString();
    qDebug() << "size:" << recvJson["size"].asInt();
    qDebug() << "md5:" << recvJson["md5"].asCString();

    //�������ص�����
    int size = recvJson["size"].asInt();
    int offset = recvJson.toStyledString().size() + 1;
    QByteArray content_ba = str_ba.mid(offset);
    qDebug() <<"downseg len: "<< content_ba.length();
    if(size != content_ba.length()){
        MyMessageBox::critical("����", "����Ƭ�γ��ȴ���");
        return;
    }
    else if(size == 0){
        MyMessageBox::critical("����", "���س���Ϊ0��");
        return;
    }
    //## ��У��Ƭ�ε�md5
    if(m_startbit + size > m_total_len){       //���س���
        MyMessageBox::critical("����", "���س��ȴ���");
        return;
    }

    //�����ļ�Ƭ��
    downfileSeg(content_ba);

    m_startbit += size;    //����������ɣ�������ʼλ��
    if(m_startbit == m_total_len){        //�������
        QFileInfo file_info(m_filepath);
        int total_len = file_info.size();
        int base_len = m_pFolder->getRootDir().length() + 1;
        QString status_str = m_filepath.mid(base_len)
                + CStr2LocalQStr(" *�ļ�������ɣ�");
        ui->lnStatus->setText(status_str);
        ui->progStatus->setValue(100);
        QString prog_str = getByteNumRatio(total_len, total_len);
#if 1
        MyMessageBox::information("��ʾ", "������ɣ�");
#endif
        qDebug() <<"parse downfileseg finish clear!!!";
        clearUpfile();

        //m_pFolder->SyncQDequeue();      //������ɣ�����
        return;
    }
    //�������سɹ���������һ��
    downfileNextSeg();
}

//ɾ���ļ�
void MainWindow::parseJsonRmfileOK(const Json::Value &recvJson)
{
    /*******************************
     "function": "rmfileok"
     ******************************/
    qDebug() << "function: "<< recvJson["function"].asCString();
    m_pFolder->SyncQDequeue();      //������ɣ�����
}

//����Ŀ¼
void MainWindow::parseJsonMkdirOK(const Json::Value &recvJson)
{
    /*******************************
     "function": "mkdirok"
     ******************************/
    qDebug() << "function: "<< recvJson["function"].asCString();
    m_pFolder->SyncQDequeue();      //������ɣ�����
}

//ɾ��Ŀ¼���������Ѻ�ɾ���ļ��ϲ���
void MainWindow::parseJsonRmdirOK(const Json::Value &recvJson)
{
    /*******************************
     "function": "rmdirok"
     ******************************/
    qDebug() << "function: "<< recvJson["function"].asCString();
    m_pFolder->SyncQDequeue();      //������ɣ�����
}

//�������ݣ����ı���
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

//��������
void MainWindow::recvData()
{
    QByteArray str_ba = m_server_sock->readAll();
    qDebug() <<"recvData len: "<< str_ba.length();

    //����
    unsigned short len, len0, len1;
    len0 = (unsigned short)(uchar)str_ba[0];
    len1 = (unsigned short)(uchar)str_ba[1];
    len = len0 + (len1 << 8);

    //ѭ����
    while(str_ba.length() - 2 < len){
        str_ba += m_server_sock->readAll();
    }
    qDebug() <<"B0: "<< len0 <<", B1: " << len1 <<", len: "<< len;
    //����
    str_ba = str_ba.mid(2, len);

#ifdef DEBUG_UPSEG
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

//��ע��ҳ��
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
    if(!isLoginUser())
        return;

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

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_REGISTER;
    sendData(sendba);     //��������
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

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_LOGIN;
    sendData(sendba);     //��������
}

//�˳���¼
void MainWindow::sendDataLogout()
{
    //�û�δ��¼
    if(!isLoginUser())
        return;
    qDebug() <<"logout...";
    qDebug() <<"username: "<< m_username;
    qDebug() <<"userid: "<< m_userid;

    Json::Value sendJson;
    sendJson["function"] = "logout";
    sendJson["userid"] = m_userid;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    clearUserid();    //�÷Ƿ�id

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_WAIT;
    sendData(sendba);     //��������
}

//�ֶ��ϴ��ļ�
void MainWindow::upfileBySeg()
{
    if(m_filepath.length() == 0){
        MyMessageBox::critical("����", "δѡ���ļ���");
        m_recv_status = STAT_WAIT;
        return;
    }
    QFileInfo file_info(m_filepath);
    qint64 total_len = file_info.size();
#ifdef DEBUG_UPSEG
    qDebug() << CStr2LocalQStr("�ļ����ȣ�") + QString::number(total_len);
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
        qDebug() << CStr2LocalQStr("�ѷ���") << QString::number(m_start_bit)
                 << CStr2LocalQStr("�ֽڣ�ʣ��") << QString::number(remain_len)
                 << CStr2LocalQStr("�ֽ�");
        */
        QTime qtime_1 = QTime::currentTime();
        if(qtime_1.second() != qtime_0.second()){
            qtime_0 = qtime_1;
            qDebug() << CStr2LocalQStr("") << QString::number(m_startbit)
                     << CStr2LocalQStr("�ֽڣ�ʣ��") << QString::number(remain_len)
                     << CStr2LocalQStr("�ֽ�");

            //## ������
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
        MyMessageBox::critical("����", "�����ϴ�0�ֽڣ�");
        m_recv_status = STAT_WAIT;
    }
}

//�ϴ��ļ�
void MainWindow::sendDataUpfile(const QString &file_path)
{
    qDebug() << "sendDataUpfile" << file_path;

    //�û�δ��¼
    if(!isLoginUser())
        return;
    QString full_path = m_pFolder->getRootDir() + "/" + file_path;
    m_filepath = full_path;
    //QMessageBox::information(nullptr, "info", full_path);

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

    QByteArray sendba = QStr2LocalBa(sendbuf);

    m_recv_status = STAT_UPFILE;
    sendData(sendba);     //��������
}

//�����ļ�Ƭ��
void MainWindow::sendDataUpfileseg(const QString &file_path, qint64 start_bit, int len)
{
    //�û�δ��¼
    if(!isLoginUser())
        return;
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

    m_startbit += seg_ba.length();

    //QByteArray���ֽ���������ֱ��תQString������'\0'֮�󶼻ᱻ����
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

    //### ��int������ַ�
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

    sendFileData(sendba, seg_ba);     //��������
}

//����Ŀ¼
void MainWindow::sendDataAskAllPath()
{
    /********************************
    "function": "askallpath"
     *******************************/
    qDebug() << "askallpath";

    //�û�δ��¼
    if(!isLoginUser())
        return;
    Json::Value sendJson;
    sendJson["function"] = "askallpath";

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_WAIT;
    sendData(sendba);     //��������

}

//�����ļ�
void MainWindow::sendDataDownfile(const QString &file_path)
{
    /********************************
    "function": "downfile",
    "path": "1/2/test.txt"
     *******************************/
    qDebug() << "downfile";

    //�û�δ��¼
    if(!isLoginUser())
        return;
    Json::Value sendJson;
    sendJson["function"] = "downfile";
    sendJson["path"] = file_path.toStdString();

    m_filepath = file_path;
    qDebug() << "file_path: " << m_filepath;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_DOWNSEG;
    sendData(sendba);     //��������
}

//�����ļ�Ƭ��
void MainWindow::sendDataDownfileseg(const QString &file_path, qint64 start_bit, int len)
{
    /********************************
    "function": "downfileseg",
    "fileid": 52,
    "startbit": 0,
    "size": 4096
     *******************************/
    qDebug() << "downfileseg";
    //�û�δ��¼
    if(!isLoginUser())
        return;
    Json::Value sendJson;
    sendJson["function"] = "downfileseg";
    sendJson["fileid"] = m_fileid;
    sendJson["startbit"] = start_bit;
    sendJson["size"] = len;

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_WAIT;
    sendData(sendba);     //��������
}

//�����ļ�Ƭ��
void MainWindow::downfileSeg(const QByteArray &content_ba)
{
    qint64 start_bit = m_startbit;
    QString file_path = m_filepath;
    int len = content_ba.length();


    qDebug() <<"down file seg......";
    QFile file_out(file_path);
    if(!file_out.open(QIODevice::ReadWrite)){
        qDebug() << CStr2LocalQStr("�ļ���ʧ�ܣ�") << file_path.toLocal8Bit();
        return;
    }
    qDebug() <<"*****************************************"<< len;
    qDebug() <<"filepath:" << m_filepath << " startbit:" << start_bit <<" len:"<< len;

    file_out.seek(start_bit);    //�ƶ��ļ�ָ��
    file_out.write(content_ba, len);    //д���ļ�
    file_out.close();        //�ر��ļ�
}

//�ֶ������ļ�
void MainWindow::downfileNextSeg()
{
    if(m_filepath.length() == 0){
        MyMessageBox::critical("����", "δѡ���ļ���");
        m_recv_status = STAT_WAIT;
        return;
    }

    const int buf_len = 4096;
    int one_recv_len = buf_len;

    qint64 total_len = m_total_len;
    qint64 remain_len = total_len - m_startbit;
    if(remain_len > 0){
        static QTime qtime_0 = QTime::currentTime();

        //�����ļ�Ƭ��
        if(remain_len < one_recv_len)
            one_recv_len = remain_len;
        sendDataDownfileseg(m_filepath, m_startbit, one_recv_len);

        QTime qtime_1 = QTime::currentTime();
        if(qtime_1.second() != qtime_0.second()){
            qtime_0 = qtime_1;
            qDebug() << CStr2LocalQStr("������") << QString::number(m_startbit)
                     << CStr2LocalQStr("�ֽڣ�ʣ��") << QString::number(remain_len)
                     << CStr2LocalQStr("�ֽ�");

            //## ������
            QString bcnt_str = getByteNumRatio(m_startbit, total_len);
            ui->lnBytes->setText(bcnt_str);
            int prog_val = 100*m_startbit/total_len;
            ui->progStatus->setValue(prog_val);
        }
        m_recv_status = STAT_WAIT;
    }
    else {
        MyMessageBox::critical("����", "�����ֽ���Ϊ0��");
        return;
    }
}

//ɾ���ļ�
void MainWindow::sendDataRmfile(const QString &file_path)
{
    qDebug() << "rmfile "<< file_path;

    //�û�δ��¼
    if(!isLoginUser())
        return;
    Json::Value sendJson;
    sendJson["function"] = "rmfile";
    sendJson["path"] = file_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);

    m_recv_status = STAT_RMFILE;
    sendData(sendba);     //��������
}

//����Ŀ¼
void MainWindow::sendDataMkdir(const QString &dir_path)
{
    qDebug() << "mkdir "<< dir_path.toLocal8Bit();

    //�û�δ��¼
    //if(!isLoginUser())
    //    return;
    Json::Value sendJson;
    sendJson["function"] = "mkdir";
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);

    //### �������ı���ת��
#if 1
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
    sendData(sendba);     //��������
}

//ɾ��Ŀ¼
void MainWindow::sendDataRmdir(const QString &dir_path)
{
    qDebug() << "rmdir "<< dir_path;

    //�û�δ��¼
    if(!isLoginUser())
        return;
    Json::Value sendJson;
    //sendJson["function"] = "rmdir";
    sendJson["function"] = "rmfile";    //ɾ�ļ���ɾĿ¼����ϲ�
    sendJson["path"] = dir_path.toStdString();

    QString sendbuf = sendJson.toStyledString().data();
    ui->txtSend->setText(sendbuf);

    QByteArray sendba = QStr2LocalBa(sendbuf);
    m_recv_status = STAT_RMDIR;
    sendData(sendba);     //��������
}

//���ڹر�
void MainWindow::closeEvent(QCloseEvent *event)
{
    //���û�Ϊ��¼״̬�������˳��ź�
    if(m_userid >= 0){
        sendDataLogout();
    }
    else {
        //sendDataLogout();
        qDebug() << CStr2LocalQStr("�û����˳�");
    }
    event->accept();
    qDebug() << CStr2LocalQStr("�����ڹرգ�");
}
