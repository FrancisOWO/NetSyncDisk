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
    //���ļ�
    QFile file_in(file_path);
    if(!file_in.open(QIODevice::ReadOnly)){
        qDebug() << CStr2LocalQStr("�ļ������ڣ�");
        return "";
    }

    //�û������ֿ��ȡ�ļ�
    int buf_len = 4096;
    quint64 total_len = file_in.size();
    quint64 remain_len = total_len;
    QByteArray read_buf;
    read_buf.resize(buf_len);
    quint64 one_read_len = buf_len;

    //����md5
    QCryptographicHash hash_md5(QCryptographicHash::Md5);
    while(remain_len > 0){
        if(remain_len < one_read_len)
            one_read_len = remain_len;
        read_buf = file_in.read(one_read_len);
        hash_md5.addData(read_buf);
        remain_len -= one_read_len;
    }
    //�ر��ļ�
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
    //system(create_cmd.toStdString().c_str()); //ע��·���е�'/'
    //return true;
    int pos = rel_path.lastIndexOf('/');
    QString dir_path = rel_path.mid(0, pos);
    //Ŀ¼�����ڣ��򴴽�Ŀ¼
    QString cmd_str = "md \"" + dir_path.replace("/", "\\") + "\"";
    qDebug() << cmd_str.toLocal8Bit();
    system(cmd_str.toLocal8Bit());
    //�漰�����е����⣬�˴��贴���ļ�
    fstream fout(rel_path.toLocal8Bit(), ios::out | ios::binary);
    fout.close();

#if 0
    QStringList split_dir = rel_path.split('/');
    int split_len = split_dir.length();
    qDebug() << "split len: "<< split_len <<" "<< rel_path;
    if(split_len < 1)
        return false;
    //�ݹ鴴���ļ���
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

//�û��� ���ȼ��
bool ULimits::checkUnameLen(const QString &username)
{
    int len = username.length();
    if(len < uname_min_len || len > uname_max_len)
        return false;
    return true;
}

//���� ���ȼ��
bool ULimits::checkPwdLen(const QString &password)
{
    int len = password.length();
    if(len < pwd_min_len || len > pwd_max_len)
        return false;
    return true;
}

//�û��� �ַ����
bool ULimits::checkUnameCh(const QString &username)
{
    int len = username.length();
    bool chk_ok = 1;

    //����Ƿ�����ĸ��ͷ
    if(!((username[0] >= 'a' && username[0] <= 'z') ||
         (username[0] >= 'A' && username[0] <= 'Z')))
        chk_ok = 0;
    //����Ƿ�������֡���ĸ���»���������ַ�
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

//���� �ַ����
bool ULimits::checkPwdCh(const QString &password)
{
    int len = password.length();
    bool chk_ok = 1;
    bool num_chk = 0, letter_chk = 0, udl_chk = 0;

    //����Ƿ��������/��ĸ/�»���
    for(int i = 0; i < len; i++){
        if(password[i] >= '0' && password[i] <= '9')    //����
            num_chk = 1;
        else if((password[i] >= 'a' && password[i] <= 'z') ||
                (password[i] >= 'A' && password[i] <= 'Z')) //��ĸ
            letter_chk = 1;
        else if(password[i] == '_')     // �»���
            udl_chk = 1;
        else {
            chk_ok = 0;
            break;
        }
    }
    chk_ok = chk_ok && num_chk && letter_chk && udl_chk;
    return chk_ok;
}

//�û��� �Ϸ��Լ��
int ULimits::checkUname(const QString &username)
{
    if(!checkUnameLen(username))
        return UNAME_ERRLEN;
    else if(!checkUnameCh(username))
        return UNAME_ERRCH;

    return UNAME_OK;
}

//���� �Ϸ��Լ��
int ULimits::checkPwd(const QString &password)
{
    if(!checkPwdLen(password))
        return PWD_ERRLEN;
    else if(!checkPwdCh(password))
        return PWD_ERRCH;

    return PWD_OK;
}
