#ifndef TOOLS_H
#define TOOLS_H
#include <QString>

QString Num2ByteNum(const int &num);

QString getByteNumRatio(const int &num1, const int &num2);

QString CStr2LocalQStr(const char *str);

QString QBa2MD5(const QByteArray &str);

QString QStr2MD5(const QString &str);

QString getFileMD5(const QString &file_path);

class MyMessageBox {
public:
    static void information(const char *title, const char *info);
    static void critical(const char *title, const char *info);
    static void warning(const char *title, const char *info);
};

class ULimits {
public:
    static const int uname_min_len = 6;
    static const int uname_max_len = 20;
    static const int pwd_min_len = 8;
    static const int pwd_max_len = 16;

    static const int PWD_OK = 0;
    static const int PWD_ERRLEN = 1;
    static const int PWD_ERRCH = 2;

    static const int UNAME_OK = 0;
    static const int UNAME_ERRLEN = 1;
    static const int UNAME_ERRCH = 2;

public:
    static bool checkUnameLen(const QString &uname);
    static bool checkPwdLen(const QString &pwd);

    static bool checkUnameCh(const QString &uname);
    static bool checkPwdCh(const QString &pwd);

    static int checkUname(const QString &uname);
    static int checkPwd(const QString &pwd);
};

#endif // TOOLS_H
