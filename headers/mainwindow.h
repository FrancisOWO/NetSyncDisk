#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QString>

#include "form_register.h"
#include "form_login.h"
#include "form_folder.h"

#include "json.h"

class QTcpSocket;
class QCloseEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:
    static const int STAT_ERROR     = -1;
    static const int STAT_WAIT      = 0;
    static const int STAT_REGISTER  = 1;
    static const int STAT_LOGIN     = 2;
    static const int STAT_LOGOUT    = 3;
    static const int STAT_UPFILE    = 4;
    static const int STAT_UPSEG     = 5;
    static const int STAT_RMFILE    = 6;
    static const int STAT_MKDIR     = 7;
    static const int STAT_RMDIR     = 8;
    static const int STAT_DOWNSEG   = 9;


    static const int CONN_NO    = 0;
    static const int CONN_ING   = 1;
    static const int CONN_OK    = 2;

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

    qint64 m_startbit;
    qint64 m_total_len;

    QString m_username;
    QString m_filepath;

private:
    void InitMembers();
    void InitConnections();
    void InitSocket();
    void DestroySocket();

    int getRealpathLen(const QString &path);

    bool isConnected();
    bool isLoginUser();
    void setConnectStatus(int status);

    void setLoginUI();
    void setNotLoginUI();

    void renameFileWithoutTmp();

    void WriteConnectLog(const char *str);
    void WriteConnectLog(const QString &str);

    void WriteConnectRec(const QString &local_dir, const QString &remote_dir);   //绑定记录

    //Json解析
    void parseJson(const QByteArray &str_ba);

    void parseJsonRegister(const Json::Value &recvJson);
    void parseJsonLogin(const Json::Value &recvJson);

    void parseJsonUpfile(const Json::Value &recvJson);
    void parseJsonUpfileseg(const Json::Value &recvJson);

    void parseJsonRmfileOK(const Json::Value &recvJson);
    void parseJsonMkdirOK(const Json::Value &recvJson);
    void parseJsonRmdirOK(const Json::Value &recvJson);

    void parseJsonAskAllPath(const Json::Value &recvJson);
    void parseJsonDownfile(const Json::Value &recvJson);
    void parseJsonDownfileseg(const Json::Value &recvJson, const QByteArray &str_ba);

private Q_SLOTS:
    void setUsername();
    void clearUsername();
    void clearUserid();
    void clearUser();

    void clearUpfile();

    //绑定目录
    void WriteBindLog(const QString &local_dir, const QString &remote_dir);

    void connectServer();       //连接服务器
    void disconnectServer();    //断开连接

    void sendFileData(const QByteArray &json_ba, const QByteArray &content_ba);
    void sendData(const QByteArray &content_ba);    //发送数据
    //void sendData(const QString &content);    //发送数据
    void sendDataFromBox();    //发送数据（读文本框）
    void recvData();    //接收数据
    void recvDataFromBox();     //接收数据（读文本框）

    void openRegisterPage();
    void openLoginPage();
    void openFolderPage();

    void sendDataRegister();    //发送注册数据
    void sendDataLogin();       //发送登录数据
    void sendDataLogout();      //发送登出数据

    void upfileBySeg();         //分段上传文件

    void sendDataUpfile(const QString &file_path);      //发送上传文件数据
    void sendDataUpfileseg(const QString &file_path, qint64 m_startbit, int len);    //发送文件片段数据

    void downfileSeg(const QByteArray &content_ba);     //下载文件片段
    void downfileNextSeg();     //下载下一段文件

    void sendDataRmfile(const QString &file_path);  //删除文件
    void sendDataMkdir(const QString &dir_path);    //创建目录
    void sendDataRmdir(const QString &dir_path);    //删除目录

    void sendDataAskAllPath();      //请求目录
    void sendDataDownfile(const QString &file_path);    //下载文件
    void sendDataDownfileseg(qint64 startbit, int file_id, int len);    //下载文件片段

};
#endif // MAINWINDOW_H
