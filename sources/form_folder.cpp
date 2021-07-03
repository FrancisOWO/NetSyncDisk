#include "form_folder.h"
#include "ui_form_folder.h"

#include <QStringList>
#include <QMessageBox>
#include <QCheckBox>

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFileDialog>

#include <QDateTime>
#include <QTime>

#include <QDebug>

#include <iostream>
#include <vector>
#include <string>
using namespace std;

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
    m_enq_enable = 1;
    m_autoUpd_flag = 0;
    m_is_banded = 0;

    resetUserid();  //清userid

    ui->treeFolder->setHeaderHidden(true);      //隐藏表头
    ui->treeRemote->setHeaderHidden(true);

}

void FormFolder::InitConnections()
{
    //选择目录
    connect(ui->pbtnChoLocalDir, SIGNAL(clicked()), this, SLOT(chooseRootDir()));
    connect(ui->pbtnBandLocalDir, SIGNAL(clicked()), this, SLOT(bandRootDir()));
    connect(ui->pbtnUpdFolder, SIGNAL(clicked()), this, SLOT(updateFolderTree()));
    //选择文件
    connect(ui->pbtnChooseFile, SIGNAL(clicked()), this, SLOT(chooseFile()));
    connect(ui->pbtnChoRemoteFile, SIGNAL(clicked()), this, SLOT(chooseRemoteFile()));
    //自动刷新
    connect(ui->chkAutoUpd, SIGNAL(clicked()), this, SLOT(updateAutoUpdFlag()));
    //清空队列
    connect(ui->pbtnClearQ, SIGNAL(clicked()), this, SLOT(SyncQClear()));
    //下载全部
    connect(ui->pbtnDownAll, SIGNAL(clicked()), this, SLOT(DownAll()));

    //监视目录和文件
    connect(m_pFilesysWatcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChanged(const QString &)));
    connect(m_pFilesysWatcher, SIGNAL(directoryChanged(const QString &)),
            this, SLOT(onDirChanged(const QString &)));
}

void FormFolder::chooseRootDir()
{
    QString path = QFileDialog::getExistingDirectory(
                nullptr, CStr2LocalQStr("选择本地目录"), m_root_dir);
    ui->lnRootDir->setText(path);
}

void FormFolder::bandRootDir()
{
    QString band_str;
    if(m_is_banded){    //已绑定，点击，解除绑定
        unBand();
        setBandStatus(false);
        MyMessageBox::information("提示", "解绑成功！");
        return;
    }

    QString path = ui->lnRootDir->text();
    if(path.length() == 0){
        QString title = CStr2LocalQStr("错误");
        QString info = CStr2LocalQStr("输入不能为空！");
        QMessageBox::critical(nullptr, title, info);
        return;
    }
    else if(path == m_root_dir){
        QString title = CStr2LocalQStr("提示");
        QString info = path + CStr2LocalQStr("目录未改变！");
        QMessageBox::information(nullptr, title, info);
        return;
    }

    QDir dDir(path);
    QFileInfo dirInfo(path);
    //目录不存在
    if(!dDir.exists()){
        QString title = CStr2LocalQStr("错误");
        QString info = CStr2LocalQStr("目录不存在！");
        QMessageBox::critical(nullptr, title, info);
    }
    //不是目录
    else if(!dirInfo.isDir()){
        QString title = CStr2LocalQStr("错误");
        QString info = path + CStr2LocalQStr("不是目录！");
        QMessageBox::critical(nullptr, title, info);
    }
    //目录存在
    else {
        setBandStatus(true);

        QString title = CStr2LocalQStr("提示");
        QString info = path + CStr2LocalQStr("目录绑定成功！");
        QMessageBox::information(nullptr, title, info);
        emit banded(path, "/");
        //目录监视
        m_pFilesysWatcher->removePath(m_root_dir);  //删除原目录
        m_root_dir = path;
        m_pFilesysWatcher->addPath(m_root_dir);     //添加新目录
        qDebug() << CStr2LocalQStr("监视目录") << m_root_dir;
        InitFolderTree();   //更新目录树
    }
    ui->lnRootDir->setText(m_root_dir);
}

void FormFolder::setLocalDir(const QString& dir)
{
    m_root_dir = dir;
    ui->lnRootDir->setText(m_root_dir);

    m_fcomp.loadFromConf(m_userid);

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
        //删除的文件自动被移除，removePath总是返回false
        //bool flag = m_pFilesysWatcher->removePath(path);
        /**********************************************
         * 此处不考虑文件是否为空，直接发删除命令。
         * 实际上，若文件为空，则远程目录没有该文件，无需删除。
         * 但程序运行至此，本地文件已被删除，无法获取长度。
         *********************************************/
        //## 同步：删除文件
        /*
        int start = m_root_dir.length() + 1;    //截取相对路径
        emit rmfile(path.mid(start));
        */
        SyncFileOrDir(path, SYNC_RMFILE);

        //刷新目录树
        if(m_autoUpd_flag)      //自动刷新
            updateFolderTree();
    }
    else {
        qDebug() << CStr2LocalQStr("修改文件！") << path;
        //修改后，变为空则删除，非空则上传
        int file_len = fileInfo.size();
        /*
        int start = m_root_dir.length() + 1;    //截取相对路径
        QString relt_path = path.mid(start);
        */
        if(file_len != 0){
            //## 同步：上传文件
            //emit upfile(relt_path);
            SyncFileOrDir(path, SYNC_UPFILE);
        }
        else{
            //## 同步：删除文件
            //emit rmfile(relt_path);
            SyncFileOrDir(path, SYNC_RMFILE);
        }
    }
}

void FormFolder::onDirChanged(const QString &path)
{
    //qDebug() << CStr2LocalQStr("目录变化！") << path;
    QDir dDir(path);
    static QString rmpath = "";
    if(!dDir.exists()){
        if(path == rmpath){
            //qDebug() << CStr2LocalQStr("目录已删除！") << path;
            return;
        }
        else {
            qDebug() << CStr2LocalQStr("删除目录！") << path;
            rmpath = path;
        }
        //删除的目录自动被移除，removePath总是返回false
        //bool flag =  m_pFilesysWatcher->removePath(path);
        //## 同步：删除目录
        /*
        int start = m_root_dir.length() + 1;    //截取相对路径
        emit rmdir(path.mid(start) + "/");
        */
        SyncFileOrDir(path, SYNC_RMDIR);
    }
    else {
        rmpath = "";
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
    QString abs_path = m_root_dir + "/" + file_path;
    ui->lnFilePath->setText(file_path);

    SyncFileOrDir(abs_path, SYNC_UPFILE);

    //emit upfile(file_path);
}

//选择远程目录
void FormFolder::chooseRemoteFile()
{
    //获取文件路径
    QTreeWidgetItem *pNode = ui->treeRemote->currentItem();
    QString file_path = pNode->text(0);     //远程文件路径
    //设置路径
    /*
    QString abs_path = m_root_dir + "/" + file_path;
    SyncFileOrDir(abs_path, SYNC_UPFILE);
    */
    ui->lnRemoteFilePath->setText(file_path);
    //未绑定，不同步
    if(m_root_dir.length() == 0){
        MyMessageBox::critical("错误", "未绑定本地目录，无法下载！");
        return;
    }
    int end_pos = file_path.length() - 1;
    if(file_path[end_pos] == '/'){  //目录
        file_path = m_root_dir + "/" + file_path;
        createDir(file_path);
    }
    else {      //文件
        //emit downfile(file_path);
        SyncFileOrDir(file_path, SYNC_DOWNFILE);
    }
}

void FormFolder::updateAutoUpdFlag()
{
    m_autoUpd_flag = ui->chkAutoUpd->isChecked();
    //qDebug() << "auto flag: "<< m_autoUpd_flag;
    if(m_autoUpd_flag){         //刷新目录
        updateFolderTree();
    }
}

void FormFolder::InitRemoteTree(Json::Value recvJson)
{
    qDebug() << CStr2LocalQStr("初始化远程目录");

    ui->treeRemote->clear();    //清空目录
    ui->treeRemote->setHeaderHidden(true);      //隐藏表头

    //添加根目录
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << "/");
    pRoot->setCheckState(1, Qt::Checked);
    qDebug() << m_root_dir;

    m_dir_list.clear();
    m_file_list.clear();

    qDebug() <<"--------------path";
    if(recvJson.isMember("path")){
        int path_size = recvJson["path"].size();
        for(int i = 0; i < path_size; i++){
            QString dir_path = QString::fromLocal8Bit(recvJson["path"][i].asString().c_str());
            qDebug() << dir_path;
            m_dir_list << dir_path;

            QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << dir_path);
            pChild->setCheckState(1, Qt::Checked);
            pRoot->addChild(pChild);
        }
    }
    qDebug() <<"--------------file";
    if(recvJson.isMember("file")){
        int file_size = recvJson["file"].size();
        for(int i = 0; i < file_size; i++){
            QString file_path = QString::fromLocal8Bit(recvJson["file"][i].asString().c_str());
            qDebug() << file_path;
            m_file_list << file_path;

            QTreeWidgetItem *pChild = new QTreeWidgetItem(QStringList() << file_path);
            pChild->setCheckState(1, Qt::Checked);
            pRoot->addChild(pChild);
        }
    }
    //更新视图
    ui->treeRemote->addTopLevelItem(pRoot);     //根目录加入视图
    pRoot->setExpanded(true);   //展开目录
    ui->treeRemote->update();
}

void FormFolder::InitFolderTree()
{
    qDebug() << CStr2LocalQStr("初始化目录");

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

//下载全部
void FormFolder::DownAll()
{
    if(m_root_dir.length() == 0){
        MyMessageBox::critical("错误", "未绑定本地目录，无法下载！");
        return;
    }
    MyMessageBox::information("提示", "开始下载全部！");
    int dlist_len = m_dir_list.length();
    int flist_len = m_file_list.length();
    //先建立目录，再下载文件
    for(int i = 0; i < dlist_len; i++){
        QString dir_path = m_root_dir + "/" + m_dir_list[i];
        createDir(dir_path);
    }
    for(int i = 0; i < flist_len; i++){
        QString file_path = m_file_list[i];
        SyncFileOrDir(file_path, SYNC_DOWNFILE);
    }
}

//清空队列
void FormFolder::SyncQClear()
{
    qDebug() <<"clear syncQ !!!";
    m_sync_q.clear();
    m_enq_enable = 1;

    emit cleardownq();
}

//解除绑定
void FormFolder::unBand()
{
    //取消监视
    QStringList file_list = m_pFilesysWatcher->files();
    QStringList dir_list = m_pFilesysWatcher->directories();
    for(int i = 0; i < file_list.length(); i++){
        m_pFilesysWatcher->removePath(file_list[i]);
    }
    for(int i = 0; i < dir_list.length(); i++){
        m_pFilesysWatcher->removePath(dir_list[i]);
    }
    //清空队列
    SyncQClear();
    m_file_list.clear();
    m_dir_list.clear();
    m_root_dir.clear();
    ui->treeFolder->clear();
    //清空绑定记录
    emit banded("","");

    return;
}

//用户退出时，清空目录
void FormFolder::clearTree()
{    //取消监视
    QStringList file_list = m_pFilesysWatcher->files();
    QStringList dir_list = m_pFilesysWatcher->directories();
    for(int i = 0; i < file_list.length(); i++){
        m_pFilesysWatcher->removePath(file_list[i]);
    }
    for(int i = 0; i < dir_list.length(); i++){
        m_pFilesysWatcher->removePath(dir_list[i]);
    }
    //清空队列
    SyncQClear();
    m_file_list.clear();
    m_dir_list.clear();
    m_root_dir = "";

    ui->treeFolder->clear();
    ui->treeRemote->clear();

    ui->lnRootDir->clear();
    ui->lnFilePath->clear();
    ui->lnRemoteFilePath->clear();

    return;
}

void FormFolder::setBandStatus(bool status)
{
    QString band_str;
    m_is_banded = status;

    //绑定成功，按钮变为解绑
    if(m_is_banded)
        band_str = CStr2LocalQStr("解除绑定");
    else
        band_str = CStr2LocalQStr("绑定目录");
    ui->pbtnBandLocalDir->setText(band_str);
}

void FormFolder::setUserid(int userid)
{
    m_userid = userid;
}

void FormFolder::resetUserid()
{
    m_userid = -1;
}

QString FormFolder::getModeStr(int mode)
{
    QString mode_str;
    if(mode == SYNC_MKDIR){
        mode_str = "mkdir";
    }
    else if(mode == SYNC_RMDIR){
        mode_str = "rmdir";
    }
    else if(mode == SYNC_UPFILE){
        mode_str = "upfile";
    }
    else if(mode == SYNC_RMFILE){
        mode_str = "rmfile";
    }
    else if(mode == SYNC_DOWNFILE){
        mode_str = "downfile";
    }
    return mode_str;
}

QString FormFolder::getSyncLogPath()
{
    return "userinfo/" + QString::number(m_userid) + ".sync.log";
}

void FormFolder::WriteSyncLog(const QByteArray &out_ba)
{
    QString filename = getSyncLogPath();
    QFile qfout(filename);
    if(!qfout.open(QFile::ReadWrite)){
    }
    QString time_str = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QByteArray write_ba = "[" + time_str.toLocal8Bit() + "]";
    write_ba += out_ba;
    write_ba += "\n";

    qfout.seek(qfout.size());
    qfout.write(write_ba, write_ba.length());
    qfout.close();
}

//出队
void FormFolder::SyncQDequeue()
{
    if(m_sync_q.isEmpty()){
        qDebug() << "ERROR: Q empty!!!";
        return;
    }
    StructSync temp_sync = m_sync_q.dequeue();

    qDebug() <<"dq cnt : "<< m_sync_q.count() <<" "<< temp_sync.path
            <<" "<< getModeStr(temp_sync.mode);
    //写日志
    QString mode_str = getModeStr(temp_sync.mode);
    QString out_str = temp_sync.path + " : " + mode_str + " END";
    WriteSyncLog(out_str.toLocal8Bit());

    //处理同步请求
    m_enq_enable = 1;
    ProcessSync();

    return;
}

//添加监视
void FormFolder::AddWatchPath(const QString &path)
{
    qDebug() <<"add watch " << path;
    m_pFilesysWatcher->addPath(path);
}

//取消监视
void FormFolder::RemoveWatchPath(const QString &path)
{
    qDebug() <<"remove watch " << path;
    m_pFilesysWatcher->removePath(path);
}

//处理同步请求
void FormFolder::ProcessSync()
{
    if(m_sync_q.count() == 0)
        return;
    if(m_sync_q.count() == 1)
        m_enq_enable = 1;
    if(!m_enq_enable)
        return;

    //取队首，发送请求
    StructSync temp_sync = m_sync_q.head();
    QString t_path = temp_sync.path;
    int t_mode = temp_sync.mode;

    qDebug() <<"pro q : "<< m_sync_q.count() << " "<< t_path
            << " " << getModeStr(t_mode);

    //写日志
    QString mode_str = getModeStr(temp_sync.mode);
    QString out_str = temp_sync.path + " : " + mode_str + " START";
    WriteSyncLog(out_str.toLocal8Bit());

    if(t_mode == SYNC_UPFILE || t_mode == SYNC_RMFILE || t_mode == SYNC_DOWNFILE){
        SyncFile(t_path, t_mode);
    }
    else if(t_mode == SYNC_MKDIR || t_mode == SYNC_RMDIR){
        SyncDir(t_path, t_mode);
    }
}

//同步文件或目录
void FormFolder::SyncFileOrDir(const QString &path, int mode)
{
    if(path.length() == 0)
        return;
    int last_pos = path.lastIndexOf('/') + 1;
    QString last_path = path.mid(last_pos);
    if(last_path[0] == "~" || last_path[0] == "." || path.endsWith(".tmp"))
        return;     //忽略临时文件

    //入队
    StructSync temp_sync = {path, mode};
    m_sync_q.enqueue(temp_sync);
    qDebug() <<"eq cnt : "<< m_sync_q.count() <<" "<< temp_sync.path
            <<" "<< getModeStr(temp_sync.mode);
    //处理同步请求
    m_enq_enable = 0;
    ProcessSync();
}

//同步文件
void FormFolder::SyncFile(const QString &file_path, int mode)
{
    int base_len = m_root_dir.length() + 1;
    QString rel_path = file_path.mid(base_len);
    //删除文件
    if(mode == SYNC_RMFILE){
        emit rmfile(rel_path);
        return;
    }
    //上传文件
    else if(mode == SYNC_UPFILE) {
        QFileInfo file_info(file_path);
        if(file_info.size() == 0){
            qDebug() << CStr2LocalQStr("空文件不上传！");
            QString out_str = CStr2LocalQStr("0字节文件无需上传");
            WriteSyncLog(out_str.toLocal8Bit());
            SyncQDequeue();
            return;
        }
        qDebug() << "full path:" << file_path <<", file_path: "<< rel_path;
        emit upfile(rel_path);
    }
    //下载文件
    else if(mode == SYNC_DOWNFILE){
        //path为远程路径
        emit downfile(file_path);
    }
}

//同步目录
void FormFolder::SyncDir(const QString &dir_path, int mode)
{
    int base_len = m_root_dir.length() + 1;
    QString rel_path = dir_path.mid(base_len) + "/";
    //删除目录
    if(mode == SYNC_RMDIR){
        emit rmdir(rel_path);
        return;
    }
    //创建目录
    else if(mode == SYNC_MKDIR){
        emit mkdir(rel_path);
        return;
    }
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
        //同步文件
        SyncFileOrDir(file_path, SYNC_UPFILE);

        m_last_path = file_path;
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
        //同步目录
        SyncFileOrDir(dir_path, SYNC_MKDIR);

        m_last_path = dir_path;
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
            //过滤刚下载的文件
            if(file_path != m_temp_path){
                //## 同步文件
                SyncFileOrDir(file_path, SYNC_UPFILE);
            }
            else {
                qDebug() << CStr2LocalQStr("过滤文件") << m_temp_path;
                m_temp_path = "";
            }

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
            //## 同步：新建目录
            SyncFileOrDir(dir_path, SYNC_MKDIR);
        }
    }

}
