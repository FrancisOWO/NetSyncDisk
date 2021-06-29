#include "form_folder.h"
#include "ui_form_folder.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>

#include "tools.h"

FormFolder::FormFolder(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormFolder)
{
    ui->setupUi(this);

    InitMembers();
    InitConnections();
}

FormFolder::~FormFolder()
{
    delete ui;
}


void FormFolder::InitMembers()
{
    pFilesysWatcher = new QFileSystemWatcher();

}

void FormFolder::InitConnections()
{
    connect(ui->pbtnChgRoot, SIGNAL(clicked()), this, SLOT(changeRootDir()));
    connect(ui->pbtnUpdFolder, SIGNAL(clicked()), this, SLOT(updateFolderTree()));

    connect(ui->pbtnChooseFile, SIGNAL(clicked()), this, SLOT(chooseFile()));

    //����Ŀ¼���ļ�
    connect(pFilesysWatcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChanged(const QString &)));
    connect(pFilesysWatcher, SIGNAL(directoryChanged(const QString &)),
            this, SLOT(onDirChanged(const QString &)));

}

void FormFolder::changeRootDir()
{
    QString path = ui->lnRootDir->text();
    if(path.length() == 0){
        QString title = CStr2LocalQStr("����");
        QString info = CStr2LocalQStr("���벻��Ϊ�գ�");
        QMessageBox::critical(NULL, title, info);
        return;
    }
    else if(path == root_dir){
        QString title = CStr2LocalQStr("��ʾ");
        QString info = path + CStr2LocalQStr("Ŀ¼δ�ı䣡");
        QMessageBox::information(NULL, title, info);
        return;
    }

    QDir dDir(path);
    QFileInfo dirInfo(path);
    //Ŀ¼������
    if(!dDir.exists()){
        QString title = CStr2LocalQStr("����");
        QString info = CStr2LocalQStr("Ŀ¼�����ڣ�");
        QMessageBox::critical(NULL, title, info);
    }
    //����Ŀ¼
    else if(!dirInfo.isDir()){
        QString title = CStr2LocalQStr("����");
        QString info = path + CStr2LocalQStr("����Ŀ¼��");
        QMessageBox::critical(NULL, title, info);
    }
    //Ŀ¼����
    else {
        QString title = CStr2LocalQStr("��ʾ");
        QString info = path + CStr2LocalQStr("Ŀ¼�����ɹ���");
        QMessageBox::information(NULL, title, info);
        //Ŀ¼����
        pFilesysWatcher->removePath(root_dir);  //ɾ��ԭĿ¼
        root_dir = path;
        pFilesysWatcher->addPath(root_dir);     //�����Ŀ¼
        qDebug() << CStr2LocalQStr("����Ŀ¼") << root_dir;
        updateFolderTree();     //����Ŀ¼��
    }
    ui->lnRootDir->setText(root_dir);
}

void FormFolder::setRootDir(const QString& dir)
{
    root_dir = dir;
    ui->lnRootDir->setText(root_dir);
    //Ŀ¼����
    pFilesysWatcher->addPath(root_dir);     //�����Ŀ¼
    qDebug() << CStr2LocalQStr("����Ŀ¼") << root_dir;
}

QString FormFolder::getRootDir()
{
    return root_dir;
}

void FormFolder::onFileChanged(const QString &path)
{
    qDebug() << CStr2LocalQStr("�ļ��仯��") << path;
    QFileInfo fileInfo(path);
    if(!fileInfo.exists()){
        qDebug() << CStr2LocalQStr("ɾ���ļ���");
        pFilesysWatcher->removePath(path);
    }
    else {
        qDebug() << CStr2LocalQStr("�޸��ļ���");
    }
}

void FormFolder::onDirChanged(const QString &path)
{
    qDebug() << CStr2LocalQStr("Ŀ¼�仯��") << path;
    QDir dDir(path);
    if(!dDir.exists()){
        qDebug() << CStr2LocalQStr("ɾ��Ŀ¼��");
        pFilesysWatcher->removePath(path);
    }
    else {
        qDebug() << CStr2LocalQStr("�޸�Ŀ¼��");
    }
}

void FormFolder::chooseFile()
{
    //��ȡ�ļ�·��
    QTreeWidgetItem *pNode = ui->treeFolder->currentItem();
    QString file_path = pNode->text(0);     //�ļ���
    //�ݹ�ƴ���ϼ�·��
    while(pNode->parent() != ui->treeFolder->topLevelItem(0)){
        pNode = pNode->parent();
        file_path  = pNode->text(0) + "/" + file_path;
    }
    //����·��
    ui->lnFilePath->setText(file_path);
    emit filechosen(file_path);
}

void FormFolder::updateFolderTree()
{
    qDebug() <<"ˢ��Ŀ¼";

    ui->treeFolder->clear();    //���Ŀ¼
    ui->treeFolder->setHeaderHidden(true);      //���ر�ͷ

    //��Ӹ�Ŀ¼
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << root_dir);
    pRoot->setCheckState(1, Qt::Checked);
    qDebug() << root_dir;

    //�ݹ�����¼�Ŀ¼���ļ�
    addFolderChilds(pRoot, root_dir);

    //������ͼ
    ui->treeFolder->addTopLevelItem(pRoot);     //��Ŀ¼������ͼ
    pRoot->setExpanded(true);   //չ��Ŀ¼
    ui->treeFolder->update();
}

//��ȡĿ¼�е�Ŀ¼���ļ����ݹ飩
void FormFolder::addFolderChilds(QTreeWidgetItem *pParent, const QString &absPath)
{
    QString parent_dir = absPath;
    qDebug() << "pdir : "<< parent_dir;

    QDir dDir(parent_dir);    //�¼�Ŀ¼
    QDir dFile(parent_dir);   //�¼��ļ�

    //����¼��ļ�
    dFile.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dFile.setSorting(QDir::Size | QDir::Reversed);

    QFileInfoList file_list = dFile.entryInfoList();
    qDebug() << "dir size: "<< file_list.size();
    for(int i = 0; i < file_list.size(); i++){
        QString file_path = file_list.at(i).absoluteFilePath();
        QString file_name = file_list.at(i).fileName();
        qDebug() << file_name;

        QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << file_name);
        pChild->setCheckState(1, Qt::Checked);
        pParent->addChild(pChild);

        pFilesysWatcher->addPath(file_path);        //��Ӽ���
    }

    //����¼�Ŀ¼
    QFileInfoList dir_list = dFile.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    qDebug() << "dir size: "<< dir_list.size();
    for(int i = 0; i < dir_list.size(); i++){
        QString dir_path = dir_list.at(i).absoluteFilePath();
        QString dir_name = dir_list.at(i).fileName();
        qDebug() << dir_name;

        QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << dir_name);
        pChild->setCheckState(1, Qt::Checked);
        pParent->addChild(pChild);
        addFolderChilds(pChild, dir_path);

        pFilesysWatcher->addPath(dir_path);         //��Ӽ���
    }

}
