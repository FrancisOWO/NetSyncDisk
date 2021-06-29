#include "form_folder.h"
#include "ui_form_folder.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include <QCheckBox>

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
    m_pFilesysWatcher = new QFileSystemWatcher();

    m_autoUpd_flag = 0;

}

void FormFolder::InitConnections()
{
    connect(ui->pbtnChgRoot, SIGNAL(clicked()), this, SLOT(changeRootDir()));
    connect(ui->pbtnUpdFolder, SIGNAL(clicked()), this, SLOT(updateFolderTree()));

    connect(ui->pbtnChooseFile, SIGNAL(clicked()), this, SLOT(chooseFile()));

    connect(ui->chkAutoUpd, SIGNAL(clicked()), this, SLOT(updateAutoUpdFlag()));

    //监视目录和文件
    connect(m_pFilesysWatcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChanged(const QString &)));
    connect(m_pFilesysWatcher, SIGNAL(directoryChanged(const QString &)),
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
    else if(path == m_root_dir){
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
        m_pFilesysWatcher->removePath(m_root_dir);  //删除原目录
        m_root_dir = path;
        m_pFilesysWatcher->addPath(m_root_dir);     //添加新目录
        qDebug() << CStr2LocalQStr("监视目录") << m_root_dir;
        InitFolderTree();   //更新目录树
    }
    ui->lnRootDir->setText(m_root_dir);
}

void FormFolder::setRootDir(const QString& dir)
{
    m_root_dir = dir;
    ui->lnRootDir->setText(m_root_dir);
    //目录监视
    m_pFilesysWatcher->addPath(m_root_dir);     //添加新目录
    qDebug() << CStr2LocalQStr("监视目录") << m_root_dir;
}

QString FormFolder::getRootDir()
{
    return m_root_dir;
}

void FormFolder::onFileChanged(const QString &path)
{
    //qDebug() << CStr2LocalQStr("文件变化！") << path;
    QFileInfo fileInfo(path);
    if(!fileInfo.exists()){
        qDebug() << CStr2LocalQStr("删除文件！") << path;
        m_pFilesysWatcher->removePath(path);
        if(m_autoUpd_flag){     //自动刷新
            updateFolderTree();
        }
    }
    else {
        qDebug() << CStr2LocalQStr("修改文件！") << path;
    }
}

void FormFolder::onDirChanged(const QString &path)
{
    //qDebug() << CStr2LocalQStr("目录变化！") << path;
    QDir dDir(path);
    if(!dDir.exists()){
        qDebug() << CStr2LocalQStr("删除目录！") << path;
        m_pFilesysWatcher->removePath(path);
    }
    else {
        //qDebug() << CStr2LocalQStr("修改目录！") << path;
    }
    if(m_autoUpd_flag){     //自动刷新
        updateFolderTree();
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

void FormFolder::updateAutoUpdFlag()
{
    m_autoUpd_flag = ui->chkAutoUpd->isChecked();
    //qDebug() << "auto flag: "<< m_autoUpd_flag;
    if(m_autoUpd_flag){         //刷新目录
        updateFolderTree();
    }
}

void FormFolder::InitFolderTree()
{
    qDebug() <<"初始化目录";

    ui->treeFolder->clear();    //清空目录
    ui->treeFolder->setHeaderHidden(true);      //隐藏表头

    //添加根目录
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << m_root_dir);
    pRoot->setCheckState(1, Qt::Checked);
    qDebug() << m_root_dir;

    //递归添加下级目录和文件
    addFolderChilds(pRoot, m_root_dir);

    //更新视图
    ui->treeFolder->addTopLevelItem(pRoot);     //根目录加入视图
    pRoot->setExpanded(true);   //展开目录
    ui->treeFolder->update();
}

void FormFolder::updateFolderTree()
{
    //qDebug() << CStr2LocalQStr("刷新目录");

    ui->treeFolder->clear();    //清空目录
    ui->treeFolder->setHeaderHidden(true);      //隐藏表头

    //添加根目录
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << m_root_dir);
    pRoot->setCheckState(1, Qt::Checked);
#ifdef DEBUG_FOLDER
    qDebug() << m_root_dir;
#endif

    //递归更新下级目录和文件
    updateFolderChilds(pRoot, m_root_dir);

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
#ifdef DEBUG_FOLDER
    qDebug() << "dir size: "<< file_list.size();
#endif
    for(int i = 0; i < file_list.size(); i++){
        QString file_path = file_list.at(i).absoluteFilePath();
        QString file_name = file_list.at(i).fileName();
        qDebug() << file_name;

        QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << file_name);
        pChild->setCheckState(1, Qt::Checked);
        pParent->addChild(pChild);

        m_pFilesysWatcher->addPath(file_path);        //添加监视

    }

    //添加下级目录
    QFileInfoList dir_list = dFile.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
#ifdef DEBUG_FOLDER
    qDebug() << "dir size: "<< dir_list.size();
#endif
    for(int i = 0; i < dir_list.size(); i++){
        QString dir_path = dir_list.at(i).absoluteFilePath();
        QString dir_name = dir_list.at(i).fileName();
        qDebug() << dir_name;

        QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << dir_name);
        pChild->setCheckState(1, Qt::Checked);
        pParent->addChild(pChild);
        addFolderChilds(pChild, dir_path);

        m_pFilesysWatcher->addPath(dir_path);         //添加监视
    }

}

//更新目录中的目录和文件（递归）
void FormFolder::updateFolderChilds(QTreeWidgetItem *pParent, const QString &absPath)
{
    QString parent_dir = absPath;
#ifdef DEBUG_FOLDER
    qDebug() << "pdir : "<< parent_dir;
#endif
    QDir dDir(parent_dir);    //下级目录
    QDir dFile(parent_dir);   //下级文件

    //添加下级文件
    dFile.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dFile.setSorting(QDir::Size | QDir::Reversed);

    //当前监视列表
    QStringList watch_files = m_pFilesysWatcher->files();
    QStringList watch_dirs = m_pFilesysWatcher->directories();

    QFileInfoList file_list = dFile.entryInfoList();
#ifdef DEBUG_FOLDER
    qDebug() << "dir size: "<< file_list.size();
#endif
    for(int i = 0; i < file_list.size(); i++){
        QString file_path = file_list.at(i).absoluteFilePath();
        QString file_name = file_list.at(i).fileName();
#ifdef DEBUG_FOLDER
        qDebug() << file_name;
#endif
        QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << file_name);
        pChild->setCheckState(1, Qt::Checked);
        pParent->addChild(pChild);

        //尚未监视，则添加监视
        if(!watch_files.contains(file_path)){
            qDebug() << CStr2LocalQStr("添加监视文件！") << file_path;
            m_pFilesysWatcher->addPath(file_path);        //添加监视
        }

    }

    //添加下级目录
    QFileInfoList dir_list = dFile.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
#ifdef DEBUG_FOLDER
    qDebug() << "dir size: "<< dir_list.size();
#endif
    for(int i = 0; i < dir_list.size(); i++){
        QString dir_path = dir_list.at(i).absoluteFilePath();
        QString dir_name = dir_list.at(i).fileName();
#ifdef DEBUG_FOLDER
        qDebug() << dir_name;
#endif
        QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << dir_name);
        pChild->setCheckState(1, Qt::Checked);
        pParent->addChild(pChild);
        updateFolderChilds(pChild, dir_path);       //更新目录

        //尚未监视，则添加监视
        if(!watch_dirs.contains(dir_path)){
            qDebug() << CStr2LocalQStr("添加监视目录！") << dir_path;
            m_pFilesysWatcher->addPath(dir_path);        //添加监视
        }
    }

}
