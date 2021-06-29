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

    QString m_username;
    QString m_file_path;

private:
    void InitMembers();
    void InitConnections();

    void InitSocket();

    void parseLoginJson(const QString &str);
    void parseUpfileJson(const QString &str);

private slots:
    void connectServer();       //���ӷ�����
    void disconnectServer();    //�Ͽ�����

    void sendData();    //��������
    void recvData();    //��������

    void openRegisterPage();
    void openLoginPage();
    void openFolderPage();

    void sendRegisterData();    //����ע������
    void sendLoginData();       //���͵�¼����
    void sendLogoutData();      //���͵ǳ�����

    void upfileBySeg();         //�ֶ��ϴ��ļ�

    void sendUpfileData(const QString &m_file_path);      //�����ϴ��ļ�����
    void sendUpfilesegData(const QString &m_file_path, qint64 start_bit, int len);    //�����ļ�Ƭ������

    void setUsername();

};
#endif // MAINWINDOW_H
