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
    void updateFolderTree();

private:
    Ui::FormFolder *ui;
    QString root_dir;

    QFileSystemWatcher *pFilesysWatcher;

private:
    void InitMembers();
    void InitConnections();

    void addFolderChilds(QTreeWidgetItem *pParent, const QString &absPath);

private slots:
    void changeRootDir();

    void onFileChanged(const QString &path);
    void onDirChanged(const QString &path);

    void chooseFile();

signals:
    void filechosen(const QString &file_path);

};

#endif // FORM_FOLDER_H
