#ifndef FORM_FOLDER_H
#define FORM_FOLDER_H

#include <QWidget>
#include <QString>
#include <QQueue>
#include "json.h"

class QFileSystemWatcher;
class QTreeWidgetItem;

namespace Ui {
class FormFolder;
}

struct StructSync {
    QString path;
    int mode;
};

class FormFolder : public QWidget
{
    Q_OBJECT

public:
    explicit FormFolder(QWidget *parent = nullptr);
    ~FormFolder();

    void setLocalDir(const QString& dir);
    QString getRootDir();

public:
    static const int SYNC_UPFILE    = 0;
    static const int SYNC_RMFILE    = 1;
    static const int SYNC_DOWNFILE  = 2;

    static const int SYNC_MKDIR     = 3;
    static const int SYNC_RMDIR     = 4;

    QString m_last_path;
    QString m_temp_path;

public:
    void setUserid(int userid);
    void resetUserid();

    void WriteSyncLog(const QByteArray &out_ba);

    QString getModeStr(int mode);
    void SyncQDequeue();

    void AddWatchPath(const QString &path);
    void RemoveWatchPath(const QString &path);

public Q_SLOTS:
    void SyncQClear();

    void InitRemoteTree(Json::Value recvJson);

    void InitFolderTree();
    void updateFolderTree();

    void DownAll();

private:
    Ui::FormFolder *ui;
    QString m_root_dir;
    bool m_autoUpd_flag;
    int m_userid;

    QStringList m_dir_list;
    QStringList m_file_list;

    QFileSystemWatcher *m_pFilesysWatcher;

    QQueue<StructSync> m_sync_q;
    bool m_enq_enable;

private:
    void InitMembers();
    void InitConnections();

    void ProcessSync();

    void SyncFileOrDir(const QString &path, int mode);
    void SyncFile(const QString &file_path, int mode = SYNC_UPFILE);
    void SyncDir(const QString &dir_path, int mode = SYNC_MKDIR);

    void addFolderChilds(QTreeWidgetItem *pParent, const QString &absPath);
    void updateFolderChilds(QTreeWidgetItem *pParent, const QString &absPath);

private Q_SLOTS:

    void chooseRootDir();
    void changeRootDir();

    void onFileChanged(const QString &path);
    void onDirChanged(const QString &path);

    void chooseFile();
    void chooseRemoteFile();

    void updateAutoUpdFlag();

Q_SIGNALS:
    void bound(const QString &local_dir, const QString &remote_dir);

    void upfile(const QString &file_path);

    //## ���ļ���ͬ��������mkfile
    //void mkfile(const QString &file_path);    //�½��ļ�
    void rmfile(const QString &file_path);      //ɾ���ļ�

    void mkdir(const QString &dir_path);        //�½�Ŀ¼
    void rmdir(const QString &dir_path);        //ɾ��Ŀ¼

    void downfile(const QString &file_path);    //�����ļ�

};

#endif // FORM_FOLDER_H
