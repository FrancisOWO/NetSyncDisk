#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QString>
#include <QCloseEvent>

#include "form_register.h"
#include "form_login.h"
#include "form_folder.h"

#include "json.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setPRegister(FormRegister *value);

public:
    static const int STAT_WAIT      = 0;
    static const int STAT_REGISTER  = 1;
    static const int STAT_LOGIN     = 2;
    static const int STAT_UPFILE    = 3;
    static const int STAT_UPSEG     = 4;
    static const int STAT_RMFILE    = 5;
    static const int STAT_MKDIR     = 6;
    static const int STAT_RMDIR     = 7;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    QTcpSocket *m_server_sock;

    FormRegister *m_pRegister;
    FormLogin *m_pLogin;
    FormFolder *m_pFolder;

    bool m_is_connected;
    int m_recv_status;
    int m_userid;
    int m_fileid;

    qint64 m_start_bit;

    QString m_username;
    QString m_file_path;

private:
    void InitMembers();
    void InitConnections();

    void InitSocket();

    void parseLoginJson(const QString &str);
    void parseUpfileJson(const QString &str);
    void parseUpfilesegJson(const QString &str);

private slots:
    void setUsername();

    void connectServer();       //连接服务器
    void disconnectServer();    //断开连接

    void sendData();    //发送数据
    void recvData();    //接收数据

    void openRegisterPage();
    void openLoginPage();
    void openFolderPage();

    void sendDataRegister();    //发送注册数据
    void sendDataLogin();       //发送登录数据
    void sendDataLogout();      //发送登出数据

    void upfileBySeg();         //分段上传文件

    void sendDataUpfile(const QString &file_path);      //发送上传文件数据
    void sendDataUpfileseg(const QString &file_path, qint64 m_start_bit, int len);    //发送文件片段数据

    void sendDataRmfile(const QString &file_path);  //删除文件
    void sendDataMkdir(const QString &dir_path);    //创建目录
    void sendDataRmdir(const QString &dir_path);    //删除目录

};
#endif // MAINWINDOW_H
