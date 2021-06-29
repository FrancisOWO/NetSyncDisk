#ifndef FORM_FOLDER_H
#define FORM_FOLDER_H

#include <QWidget>
#include <QString>
#include <QTreeWidgetItem>
#include <QFileSystemWatcher>

namespace Ui {
class FormFolder;
}

class FormFolder : public QWidget
{
    Q_OBJECT

public:
    explicit FormFolder(QWidget *parent = nullptr);
    ~FormFolder();

    void setRootDir(const QString& dir);
    QString getRootDir();

public slots:
    void InitFolderTree();
    void updateFolderTree();

private:
    Ui::FormFolder *ui;
    QString m_root_dir;
    bool m_autoUpd_flag;

    QFileSystemWatcher *m_pFilesysWatcher;

private:
    void InitMembers();
    void InitConnections();

    void addFolderChilds(QTreeWidgetItem *pParent, const QString &absPath);
    void updateFolderChilds(QTreeWidgetItem *pParent, const QString &absPath);

private slots:
    void chooseRootDir();
    void changeRootDir();

    void onFileChanged(const QString &path);
    void onDirChanged(const QString &path);

    void chooseFile();

    void updateAutoUpdFlag();

signals:
    void upfile(const QString &file_path);

    //## ���ļ���ͬ��������mkfile
    //void mkfile(const QString &file_path);    //�½��ļ�
    void rmfile(const QString &file_path);      //ɾ���ļ�

    void mkdir(const QString &dir_path);    //�½�Ŀ¼
    void rmdir(const QString &dir_path);    //ɾ��Ŀ¼

};

#endif // FORM_FOLDER_H
