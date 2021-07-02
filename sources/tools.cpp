#include "tools.h"

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QDebug>
#include <fstream>
using namespace std;

QString Num2ByteNum(const int &num)
{
    float fnum = num;
    if(fnum < 1024){
        return QString::number(num) + "B";
    }
    fnum = fnum/1024;
    if(fnum < 1024){
        return QString::number(fnum,'f',2) + "KB";
    }
    fnum = fnum/1024;
    if(fnum < 1024){
        return QString::number(fnum,'f',2) + "MB";
    }
    fnum = fnum/1024;
    return QString::number(fnum,'f',2) + "GB";
}

QString getByteNumRatio(const int &num1, const int &num2)
{
    return Num2ByteNum(num1) + "/" + Num2ByteNum(num2);
}

QByteArray QStr2LocalBa(const QString &str)
{
    return str.toLocal8Bit();
}

QString CStr2LocalQStr(const char *str)
{
    return QString::fromLocal8Bit(str);
}

QString QBa2MD5(const QByteArray &barray)
{
    QByteArray ba = QCryptographicHash::hash(
                barray, QCryptographicHash::Md5);
    return ba.toHex();
}

QString QStr2MD5(const QString &str)
{
    QByteArray ba = QCryptographicHash::hash(
                str.toLatin1(), QCryptographicHash::Md5);
    return ba.toHex();
}

QString getFileMD5(const QString &file_path)
{
    //打开文件
    QFile file_in(file_path);
    if(!file_in.open(QIODevice::ReadOnly)){
        qDebug() << CStr2LocalQStr("文件不存在！");
        return "";
    }

    //用缓冲区分块读取文件
    int buf_len = 4096;
    quint64 total_len = file_in.size();
    quint64 remain_len = total_len;
    QByteArray read_buf;
    read_buf.resize(buf_len);
    quint64 one_read_len = buf_len;

    //计算md5
    QCryptographicHash hash_md5(QCryptographicHash::Md5);
    while(remain_len > 0){
        if(remain_len < one_read_len)
            one_read_len = remain_len;
        read_buf = file_in.read(one_read_len);
        hash_md5.addData(read_buf);
        remain_len -= one_read_len;
    }
    //关闭文件
    file_in.close();

    return hash_md5.result().toHex();
}

bool createDir(const QString &rel_path)
{
    QString str = rel_path;
    QString str1 = "md \"" + str.replace("/", "\\") + "\"";
    qDebug() << str1.toLocal8Bit();
    system(str1.toLocal8Bit());

    return true;
}

bool createFile(const QString &rel_path)
{
    //QString create_cmd = "md " + rel_path;
    //system(create_cmd.toStdString().c_str()); //注意路径中的'/'
    //return true;
    int pos = rel_path.lastIndexOf('/');
    QString dir_path = rel_path.mid(0, pos);
    //目录不存在，则创建目录
    QString cmd_str = "md \"" + dir_path.replace("/", "\\") + "\"";
    qDebug() << cmd_str.toLocal8Bit();
    system(cmd_str.toLocal8Bit());
    //涉及到并行的问题，此处需创建文件
    fstream fout(rel_path.toLocal8Bit(), ios::out | ios::binary);
    fout.close();

#if 0
    QStringList split_dir = rel_path.split('/');
    int split_len = split_dir.length();
    qDebug() << "split len: "<< split_len <<" "<< rel_path;
    if(split_len < 1)
        return false;
    //递归创建文件夹
    else if(split_len > 1){
        //int dir_len = rel_path.length() - split_dir.last().length();
        //QString dir_path = rel_path.mid(0, dir_len);
        //creatDir
        int dir_cnt = split_dir.count() - 1;
        QString temp_path;
        for(int i = 0; i < dir_cnt; i++){
            temp_path += split_dir[i] + "/";
            qDebug() << "create dir" << temp_path;
            QDir temp_dir(temp_path);
            if(!temp_dir.exists()){
                if(!temp_dir.mkdir(temp_path))
                    return false;
            }
        }
    }
    qDebug() << "file_path:" << rel_path;
#endif
    return true;
}

void MyMessageBox::information(const char *title, const char *info)
{
    QString qtitle = CStr2LocalQStr(title);
    QString qinfo = CStr2LocalQStr(info);
    QMessageBox::information(nullptr, qtitle, qinfo);

}

void MyMessageBox::critical(const char *title, const char *info)
{
    QString qtitle = CStr2LocalQStr(title);
    QString qinfo = CStr2LocalQStr(info);
    QMessageBox::critical(nullptr, qtitle, qinfo);
}

void MyMessageBox::warning(const char *title, const char *info)
{
    QString qtitle = CStr2LocalQStr(title);
    QString qinfo = CStr2LocalQStr(info);
    QMessageBox::warning(nullptr, qtitle, qinfo);
}

//用户名 长度检查
bool ULimits::checkUnameLen(const QString &username)
{
    int len = username.length();
    if(len < uname_min_len || len > uname_max_len)
        return false;
    return true;
}

//密码 长度检查
bool ULimits::checkPwdLen(const QString &password)
{
    int len = password.length();
    if(len < pwd_min_len || len > pwd_max_len)
        return false;
    return true;
}

//用户名 字符检查
bool ULimits::checkUnameCh(const QString &username)
{
    int len = username.length();
    bool chk_ok = 1;

    //检查是否以字母开头
    if(!((username[0] >= 'a' && username[0] <= 'z') ||
         (username[0] >= 'A' && username[0] <= 'Z')))
        chk_ok = 0;
    //检查是否包含数字、字母、下划线以外的字符
    else {
        for(int i = 0; i < len; i++){
            if(!((username[i] >= '0' && username[i] <= '9') ||
                 (username[i] >= 'a' && username[i] <= 'z') ||
                 (username[i] >= 'A' && username[i] <= 'Z') || (username[i] == '_'))){
                chk_ok = 0;
                qDebug() <<"ERR CH";
                break;
            }
        }
    }
    return chk_ok;
}

//密码 字符检查
bool ULimits::checkPwdCh(const QString &password)
{
    int len = password.length();
    bool chk_ok = 1;
    bool num_chk = 0, letter_chk = 0, udl_chk = 0;

    //检查是否包含数字/字母/下划线
    for(int i = 0; i < len; i++){
        if(password[i] >= '0' && password[i] <= '9')    //数字
            num_chk = 1;
        else if((password[i] >= 'a' && password[i] <= 'z') ||
                (password[i] >= 'A' && password[i] <= 'Z')) //字母
            letter_chk = 1;
        else if(password[i] == '_')     // 下划线
            udl_chk = 1;
        else {
            chk_ok = 0;
            break;
        }
    }
    chk_ok = chk_ok && num_chk && letter_chk && udl_chk;
    return chk_ok;
}

//用户名 合法性检查
int ULimits::checkUname(const QString &username)
{
    if(!checkUnameLen(username))
        return UNAME_ERRLEN;
    else if(!checkUnameCh(username))
        return UNAME_ERRCH;

    return UNAME_OK;
}

//密码 合法性检查
int ULimits::checkPwd(const QString &password)
{
    if(!checkPwdLen(password))
        return PWD_ERRLEN;
    else if(!checkPwdCh(password))
        return PWD_ERRCH;

    return PWD_OK;
}
