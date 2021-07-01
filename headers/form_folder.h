#ifndef FORM_FOLDER_H
#define FORM_FOLDER_H

#include <QWidget>
#include <QString>
#include <QTreeWidgetItem>
#include <QFileSystemWatcher>
#include <QQueue>
#include "json.h"

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

    void setRootDir(const QString& dir);
    QString getRootDir();

public:
    static const int SYNC_UPFILE = 0;
    static const int SYNC_RMFILE = 1;
    static const int SYNC_MKDIR = 2;
    static const int SYNC_RMDIR = 3;

    QString m_last_path;

public:
    QString getModeStr(int mode);
    void SyncQDequeue();

public slots:
    void SyncQClear();

    void InitRemoteTree(Json::Value recvJson);

    void InitFolderTree();
    void updateFolderTree();

private:
    Ui::FormFolder *ui;
    QString m_root_dir;
    bool m_autoUpd_flag;

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

private slots:

    void chooseRootDir();
    void changeRootDir();

    void onFileChanged(const QString &path);
    void onDirChanged(const QString &path);

    void chooseFile();
    void chooseRemoteFile();

    void updateAutoUpdFlag();

signals:
    void upfile(const QString &file_path);

    //## 空文件不同步，无需mkfile
    //void mkfile(const QString &file_path);    //新建文件
    void rmfile(const QString &file_path);      //删除文件

    void mkdir(const QString &dir_path);        //新建目录
    void rmdir(const QString &dir_path);        //删除目录

    void downfile(const QString &file_path);    //下载文件

};

#endif // FORM_FOLDER_H
