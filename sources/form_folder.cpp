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

    //监视目录和文件
    connect(pFilesysWatcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChanged(const QString &)));
    connect(pFilesysWatcher, SIGNAL(directoryChanged(const QString &)),
            this, SLOT(onDirChanged(const QString &)));

}

void FormFolder::changeRootDir()
{
    QString path = ui->lnRootDir->text();
    if(path.length() == 0){
        QString title = CStr2LocalQStr("错误");
        QString info = CStr2LocalQStr("输入不能为空！");
        QMessageBox::critical(NULL, title, info);
        return;
    }
    else if(path == root_dir){
        QString title = CStr2LocalQStr("提示");
        QString info = path + CStr2LocalQStr("目录未改变！");
        QMessageBox::information(NULL, title, info);
        return;
    }

    QDir dDir(path);
    QFileInfo dirInfo(path);
    //目录不存在
    if(!dDir.exists()){
        QString title = CStr2LocalQStr("错误");
        QString info = CStr2LocalQStr("目录不存在！");
        QMessageBox::critical(NULL, title, info);
    }
    //不是目录
    else if(!dirInfo.isDir()){
        QString title = CStr2LocalQStr("错误");
        QString info = path + CStr2LocalQStr("不是目录！");
        QMessageBox::critical(NULL, title, info);
    }
    //目录存在
    else {
        QString title = CStr2LocalQStr("提示");
        QString info = path + CStr2LocalQStr("目录更换成功！");
        QMessageBox::information(NULL, title, info);
        //目录监视
        pFilesysWatcher->removePath(root_dir);  //删除原目录
        root_dir = path;
        pFilesysWatcher->addPath(root_dir);     //添加新目录
        qDebug() << CStr2LocalQStr("监视目录") << root_dir;
        updateFolderTree();     //更新目录树
    }
    ui->lnRootDir->setText(root_dir);
}

void FormFolder::setRootDir(const QString& dir)
{
    root_dir = dir;
    ui->lnRootDir->setText(root_dir);
    //目录监视
    pFilesysWatcher->addPath(root_dir);     //添加新目录
    qDebug() << CStr2LocalQStr("监视目录") << root_dir;
}

QString FormFolder::getRootDir()
{
    return root_dir;
}

void FormFolder::onFileChanged(const QString &path)
{
    qDebug() << CStr2LocalQStr("文件变化！") << path;
    QFileInfo fileInfo(path);
    if(!fileInfo.exists()){
        qDebug() << CStr2LocalQStr("删除文件！");
        pFilesysWatcher->removePath(path);
    }
    else {
        qDebug() << CStr2LocalQStr("修改文件！");
    }
}

void FormFolder::onDirChanged(const QString &path)
{
    qDebug() << CStr2LocalQStr("目录变化！") << path;
    QDir dDir(path);
    if(!dDir.exists()){
        qDebug() << CStr2LocalQStr("删除目录！");
        pFilesysWatcher->removePath(path);
    }
    else {
        qDebug() << CStr2LocalQStr("修改目录！");
    }
}

void FormFolder::chooseFile()
{
    //获取文件路径
    QTreeWidgetItem *pNode = ui->treeFolder->currentItem();
    QString file_path = pNode->text(0);     //文件名
    //递归拼接上级路径
    while(pNode->parent() != ui->treeFolder->topLevelItem(0)){
        pNode = pNode->parent();
        file_path  = pNode->text(0) + "/" + file_path;
    }
    //设置路径
    ui->lnFilePath->setText(file_path);
    emit filechosen(file_path);
}

void FormFolder::updateFolderTree()
{
    qDebug() <<"刷新目录";

    ui->treeFolder->clear();    //清空目录
    ui->treeFolder->setHeaderHidden(true);      //隐藏表头

    //添加根目录
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << root_dir);
    pRoot->setCheckState(1, Qt::Checked);
    qDebug() << root_dir;

    //递归添加下级目录和文件
    addFolderChilds(pRoot, root_dir);

    //更新视图
    ui->treeFolder->addTopLevelItem(pRoot);     //根目录加入视图
    pRoot->setExpanded(true);   //展开目录
    ui->treeFolder->update();
}

//获取目录中的目录和文件（递归）
void FormFolder::addFolderChilds(QTreeWidgetItem *pParent, const QString &absPath)
{
    QString parent_dir = absPath;
    qDebug() << "pdir : "<< parent_dir;

    QDir dDir(parent_dir);    //下级目录
    QDir dFile(parent_dir);   //下级文件

    //添加下级文件
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

        pFilesysWatcher->addPath(file_path);        //添加监视
    }

    //添加下级目录
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

        pFilesysWatcher->addPath(dir_path);         //添加监视
    }

}
