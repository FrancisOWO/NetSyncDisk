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

    resetUserid();  //��userid

    ui->treeFolder->setHeaderHidden(true);      //���ر�ͷ
    ui->treeRemote->setHeaderHidden(true);

}

void FormFolder::InitConnections()
{
    //ѡ��Ŀ¼
    connect(ui->pbtnChoLocalDir, SIGNAL(clicked()), this, SLOT(chooseRootDir()));
    connect(ui->pbtnBandLocalDir, SIGNAL(clicked()), this, SLOT(bandRootDir()));
    connect(ui->pbtnUpdFolder, SIGNAL(clicked()), this, SLOT(updateFolderTree()));
    //ѡ���ļ�
    connect(ui->pbtnChooseFile, SIGNAL(clicked()), this, SLOT(chooseFile()));
    connect(ui->pbtnChoRemoteFile, SIGNAL(clicked()), this, SLOT(chooseRemoteFile()));
    //�Զ�ˢ��
    connect(ui->chkAutoUpd, SIGNAL(clicked()), this, SLOT(updateAutoUpdFlag()));
    //��ն���
    connect(ui->pbtnClearQ, SIGNAL(clicked()), this, SLOT(SyncQClear()));
    //����ȫ��
    connect(ui->pbtnDownAll, SIGNAL(clicked()), this, SLOT(DownAll()));

    //����Ŀ¼���ļ�
    connect(m_pFilesysWatcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChanged(const QString &)));
    connect(m_pFilesysWatcher, SIGNAL(directoryChanged(const QString &)),
            this, SLOT(onDirChanged(const QString &)));
}

void FormFolder::chooseRootDir()
{
    QString path = QFileDialog::getExistingDirectory(
                nullptr, CStr2LocalQStr("ѡ�񱾵�Ŀ¼"), m_root_dir);
    ui->lnRootDir->setText(path);
}

void FormFolder::bandRootDir()
{
    QString band_str;
    if(m_is_banded){    //�Ѱ󶨣�����������
        unBand();
        setBandStatus(false);
        MyMessageBox::information("��ʾ", "���ɹ���");
        return;
    }

    QString path = ui->lnRootDir->text();
    if(path.length() == 0){
        QString title = CStr2LocalQStr("����");
        QString info = CStr2LocalQStr("���벻��Ϊ�գ�");
        QMessageBox::critical(nullptr, title, info);
        return;
    }
    else if(path == m_root_dir){
        QString title = CStr2LocalQStr("��ʾ");
        QString info = path + CStr2LocalQStr("Ŀ¼δ�ı䣡");
        QMessageBox::information(nullptr, title, info);
        return;
    }

    QDir dDir(path);
    QFileInfo dirInfo(path);
    //Ŀ¼������
    if(!dDir.exists()){
        QString title = CStr2LocalQStr("����");
        QString info = CStr2LocalQStr("Ŀ¼�����ڣ�");
        QMessageBox::critical(nullptr, title, info);
    }
    //����Ŀ¼
    else if(!dirInfo.isDir()){
        QString title = CStr2LocalQStr("����");
        QString info = path + CStr2LocalQStr("����Ŀ¼��");
        QMessageBox::critical(nullptr, title, info);
    }
    //Ŀ¼����
    else {
        setBandStatus(true);

        QString title = CStr2LocalQStr("��ʾ");
        QString info = path + CStr2LocalQStr("Ŀ¼�󶨳ɹ���");
        QMessageBox::information(nullptr, title, info);
        emit banded(path, "/");
        //Ŀ¼����
        m_pFilesysWatcher->removePath(m_root_dir);  //ɾ��ԭĿ¼
        m_root_dir = path;
        m_pFilesysWatcher->addPath(m_root_dir);     //�����Ŀ¼
        qDebug() << CStr2LocalQStr("����Ŀ¼") << m_root_dir;
        InitFolderTree();   //����Ŀ¼��
    }
    ui->lnRootDir->setText(m_root_dir);
}

void FormFolder::setLocalDir(const QString& dir)
{
    m_root_dir = dir;
    ui->lnRootDir->setText(m_root_dir);

    m_fcomp.loadFromConf(m_userid);

    //Ŀ¼����
    m_pFilesysWatcher->addPath(m_root_dir);     //�����Ŀ¼
    qDebug() << CStr2LocalQStr("����Ŀ¼") << m_root_dir;
}

QString FormFolder::getRootDir()
{
    return m_root_dir;
}

void FormFolder::onFileChanged(const QString &path)
{
    //qDebug() << CStr2LocalQStr("�ļ��仯��") << path;
    QFileInfo fileInfo(path);
    if(!fileInfo.exists()){
        qDebug() << CStr2LocalQStr("ɾ���ļ���") << path;
        //ɾ�����ļ��Զ����Ƴ���removePath���Ƿ���false
        //bool flag = m_pFilesysWatcher->removePath(path);
        /**********************************************
         * �˴��������ļ��Ƿ�Ϊ�գ�ֱ�ӷ�ɾ�����
         * ʵ���ϣ����ļ�Ϊ�գ���Զ��Ŀ¼û�и��ļ�������ɾ����
         * �������������ˣ������ļ��ѱ�ɾ�����޷���ȡ���ȡ�
         *********************************************/
        //## ͬ����ɾ���ļ�
        /*
        int start = m_root_dir.length() + 1;    //��ȡ���·��
        emit rmfile(path.mid(start));
        */
        SyncFileOrDir(path, SYNC_RMFILE);

        //ˢ��Ŀ¼��
        if(m_autoUpd_flag)      //�Զ�ˢ��
            updateFolderTree();
    }
    else {
        qDebug() << CStr2LocalQStr("�޸��ļ���") << path;
        //�޸ĺ󣬱�Ϊ����ɾ�����ǿ����ϴ�
        int file_len = fileInfo.size();
        /*
        int start = m_root_dir.length() + 1;    //��ȡ���·��
        QString relt_path = path.mid(start);
        */
        if(file_len != 0){
            //## ͬ�����ϴ��ļ�
            //emit upfile(relt_path);
            SyncFileOrDir(path, SYNC_UPFILE);
        }
        else{
            //## ͬ����ɾ���ļ�
            //emit rmfile(relt_path);
            SyncFileOrDir(path, SYNC_RMFILE);
        }
    }
}

void FormFolder::onDirChanged(const QString &path)
{
    //qDebug() << CStr2LocalQStr("Ŀ¼�仯��") << path;
    QDir dDir(path);
    static QString rmpath = "";
    if(!dDir.exists()){
        if(path == rmpath){
            //qDebug() << CStr2LocalQStr("Ŀ¼��ɾ����") << path;
            return;
        }
        else {
            qDebug() << CStr2LocalQStr("ɾ��Ŀ¼��") << path;
            rmpath = path;
        }
        //ɾ����Ŀ¼�Զ����Ƴ���removePath���Ƿ���false
        //bool flag =  m_pFilesysWatcher->removePath(path);
        //## ͬ����ɾ��Ŀ¼
        /*
        int start = m_root_dir.length() + 1;    //��ȡ���·��
        emit rmdir(path.mid(start) + "/");
        */
        SyncFileOrDir(path, SYNC_RMDIR);
    }
    else {
        rmpath = "";
        //qDebug() << CStr2LocalQStr("�޸�Ŀ¼��") << path;
    }
    if(m_autoUpd_flag){     //�Զ�ˢ��
        updateFolderTree();
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
    QString abs_path = m_root_dir + "/" + file_path;
    ui->lnFilePath->setText(file_path);

    SyncFileOrDir(abs_path, SYNC_UPFILE);

    //emit upfile(file_path);
}

//ѡ��Զ��Ŀ¼
void FormFolder::chooseRemoteFile()
{
    //��ȡ�ļ�·��
    QTreeWidgetItem *pNode = ui->treeRemote->currentItem();
    QString file_path = pNode->text(0);     //Զ���ļ�·��
    //����·��
    /*
    QString abs_path = m_root_dir + "/" + file_path;
    SyncFileOrDir(abs_path, SYNC_UPFILE);
    */
    ui->lnRemoteFilePath->setText(file_path);
    //δ�󶨣���ͬ��
    if(m_root_dir.length() == 0){
        MyMessageBox::critical("����", "δ�󶨱���Ŀ¼���޷����أ�");
        return;
    }
    int end_pos = file_path.length() - 1;
    if(file_path[end_pos] == '/'){  //Ŀ¼
        file_path = m_root_dir + "/" + file_path;
        createDir(file_path);
    }
    else {      //�ļ�
        //emit downfile(file_path);
        SyncFileOrDir(file_path, SYNC_DOWNFILE);
    }
}

void FormFolder::updateAutoUpdFlag()
{
    m_autoUpd_flag = ui->chkAutoUpd->isChecked();
    //qDebug() << "auto flag: "<< m_autoUpd_flag;
    if(m_autoUpd_flag){         //ˢ��Ŀ¼
        updateFolderTree();
    }
}

void FormFolder::InitRemoteTree(Json::Value recvJson)
{
    qDebug() << CStr2LocalQStr("��ʼ��Զ��Ŀ¼");

    ui->treeRemote->clear();    //���Ŀ¼
    ui->treeRemote->setHeaderHidden(true);      //���ر�ͷ

    //��Ӹ�Ŀ¼
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
    //������ͼ
    ui->treeRemote->addTopLevelItem(pRoot);     //��Ŀ¼������ͼ
    pRoot->setExpanded(true);   //չ��Ŀ¼
    ui->treeRemote->update();
}

void FormFolder::InitFolderTree()
{
    qDebug() << CStr2LocalQStr("��ʼ��Ŀ¼");

    ui->treeFolder->clear();    //���Ŀ¼
    ui->treeFolder->setHeaderHidden(true);      //���ر�ͷ

    //��Ӹ�Ŀ¼
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << m_root_dir);
    pRoot->setCheckState(1, Qt::Checked);
    qDebug() << m_root_dir;

    //�ݹ�����¼�Ŀ¼���ļ�
    addFolderChilds(pRoot, m_root_dir);

    //������ͼ
    ui->treeFolder->addTopLevelItem(pRoot);     //��Ŀ¼������ͼ
    pRoot->setExpanded(true);   //չ��Ŀ¼
    ui->treeFolder->update();
}

void FormFolder::updateFolderTree()
{
    //qDebug() << CStr2LocalQStr("ˢ��Ŀ¼");

    ui->treeFolder->clear();    //���Ŀ¼
    ui->treeFolder->setHeaderHidden(true);      //���ر�ͷ

    //��Ӹ�Ŀ¼
    QTreeWidgetItem *pRoot = new QTreeWidgetItem(QStringList() << m_root_dir);
    pRoot->setCheckState(1, Qt::Checked);
#ifdef DEBUG_FOLDER
    qDebug() << m_root_dir;
#endif

    //�ݹ�����¼�Ŀ¼���ļ�
    updateFolderChilds(pRoot, m_root_dir);

    //������ͼ
    ui->treeFolder->addTopLevelItem(pRoot);     //��Ŀ¼������ͼ
    pRoot->setExpanded(true);   //չ��Ŀ¼
    ui->treeFolder->update();
}

//����ȫ��
void FormFolder::DownAll()
{
    if(m_root_dir.length() == 0){
        MyMessageBox::critical("����", "δ�󶨱���Ŀ¼���޷����أ�");
        return;
    }
    MyMessageBox::information("��ʾ", "��ʼ����ȫ����");
    int dlist_len = m_dir_list.length();
    int flist_len = m_file_list.length();
    //�Ƚ���Ŀ¼���������ļ�
    for(int i = 0; i < dlist_len; i++){
        QString dir_path = m_root_dir + "/" + m_dir_list[i];
        createDir(dir_path);
    }
    for(int i = 0; i < flist_len; i++){
        QString file_path = m_file_list[i];
        SyncFileOrDir(file_path, SYNC_DOWNFILE);
    }
}

//��ն���
void FormFolder::SyncQClear()
{
    qDebug() <<"clear syncQ !!!";
    m_sync_q.clear();
    m_enq_enable = 1;

    emit cleardownq();
}

//�����
void FormFolder::unBand()
{
    //ȡ������
    QStringList file_list = m_pFilesysWatcher->files();
    QStringList dir_list = m_pFilesysWatcher->directories();
    for(int i = 0; i < file_list.length(); i++){
        m_pFilesysWatcher->removePath(file_list[i]);
    }
    for(int i = 0; i < dir_list.length(); i++){
        m_pFilesysWatcher->removePath(dir_list[i]);
    }
    //��ն���
    SyncQClear();
    m_file_list.clear();
    m_dir_list.clear();
    m_root_dir.clear();
    ui->treeFolder->clear();
    //��հ󶨼�¼
    emit banded("","");

    return;
}

//�û��˳�ʱ�����Ŀ¼
void FormFolder::clearTree()
{    //ȡ������
    QStringList file_list = m_pFilesysWatcher->files();
    QStringList dir_list = m_pFilesysWatcher->directories();
    for(int i = 0; i < file_list.length(); i++){
        m_pFilesysWatcher->removePath(file_list[i]);
    }
    for(int i = 0; i < dir_list.length(); i++){
        m_pFilesysWatcher->removePath(dir_list[i]);
    }
    //��ն���
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

    //�󶨳ɹ�����ť��Ϊ���
    if(m_is_banded)
        band_str = CStr2LocalQStr("�����");
    else
        band_str = CStr2LocalQStr("��Ŀ¼");
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

//����
void FormFolder::SyncQDequeue()
{
    if(m_sync_q.isEmpty()){
        qDebug() << "ERROR: Q empty!!!";
        return;
    }
    StructSync temp_sync = m_sync_q.dequeue();

    qDebug() <<"dq cnt : "<< m_sync_q.count() <<" "<< temp_sync.path
            <<" "<< getModeStr(temp_sync.mode);
    //д��־
    QString mode_str = getModeStr(temp_sync.mode);
    QString out_str = temp_sync.path + " : " + mode_str + " END";
    WriteSyncLog(out_str.toLocal8Bit());

    //����ͬ������
    m_enq_enable = 1;
    ProcessSync();

    return;
}

//��Ӽ���
void FormFolder::AddWatchPath(const QString &path)
{
    qDebug() <<"add watch " << path;
    m_pFilesysWatcher->addPath(path);
}

//ȡ������
void FormFolder::RemoveWatchPath(const QString &path)
{
    qDebug() <<"remove watch " << path;
    m_pFilesysWatcher->removePath(path);
}

//����ͬ������
void FormFolder::ProcessSync()
{
    if(m_sync_q.count() == 0)
        return;
    if(m_sync_q.count() == 1)
        m_enq_enable = 1;
    if(!m_enq_enable)
        return;

    //ȡ���ף���������
    StructSync temp_sync = m_sync_q.head();
    QString t_path = temp_sync.path;
    int t_mode = temp_sync.mode;

    qDebug() <<"pro q : "<< m_sync_q.count() << " "<< t_path
            << " " << getModeStr(t_mode);

    //д��־
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

//ͬ���ļ���Ŀ¼
void FormFolder::SyncFileOrDir(const QString &path, int mode)
{
    if(path.length() == 0)
        return;
    int last_pos = path.lastIndexOf('/') + 1;
    QString last_path = path.mid(last_pos);
    if(last_path[0] == "~" || last_path[0] == "." || path.endsWith(".tmp"))
        return;     //������ʱ�ļ�

    //���
    StructSync temp_sync = {path, mode};
    m_sync_q.enqueue(temp_sync);
    qDebug() <<"eq cnt : "<< m_sync_q.count() <<" "<< temp_sync.path
            <<" "<< getModeStr(temp_sync.mode);
    //����ͬ������
    m_enq_enable = 0;
    ProcessSync();
}

//ͬ���ļ�
void FormFolder::SyncFile(const QString &file_path, int mode)
{
    int base_len = m_root_dir.length() + 1;
    QString rel_path = file_path.mid(base_len);
    //ɾ���ļ�
    if(mode == SYNC_RMFILE){
        emit rmfile(rel_path);
        return;
    }
    //�ϴ��ļ�
    else if(mode == SYNC_UPFILE) {
        QFileInfo file_info(file_path);
        if(file_info.size() == 0){
            qDebug() << CStr2LocalQStr("���ļ����ϴ���");
            QString out_str = CStr2LocalQStr("0�ֽ��ļ������ϴ�");
            WriteSyncLog(out_str.toLocal8Bit());
            SyncQDequeue();
            return;
        }
        qDebug() << "full path:" << file_path <<", file_path: "<< rel_path;
        emit upfile(rel_path);
    }
    //�����ļ�
    else if(mode == SYNC_DOWNFILE){
        //pathΪԶ��·��
        emit downfile(file_path);
    }
}

//ͬ��Ŀ¼
void FormFolder::SyncDir(const QString &dir_path, int mode)
{
    int base_len = m_root_dir.length() + 1;
    QString rel_path = dir_path.mid(base_len) + "/";
    //ɾ��Ŀ¼
    if(mode == SYNC_RMDIR){
        emit rmdir(rel_path);
        return;
    }
    //����Ŀ¼
    else if(mode == SYNC_MKDIR){
        emit mkdir(rel_path);
        return;
    }
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

        m_pFilesysWatcher->addPath(file_path);        //��Ӽ���
        //ͬ���ļ�
        SyncFileOrDir(file_path, SYNC_UPFILE);

        m_last_path = file_path;
    }

    //����¼�Ŀ¼
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

        m_pFilesysWatcher->addPath(dir_path);         //��Ӽ���
        //ͬ��Ŀ¼
        SyncFileOrDir(dir_path, SYNC_MKDIR);

        m_last_path = dir_path;
    }

}

//����Ŀ¼�е�Ŀ¼���ļ����ݹ飩
void FormFolder::updateFolderChilds(QTreeWidgetItem *pParent, const QString &absPath)
{
    QString parent_dir = absPath;
#ifdef DEBUG_FOLDER
    qDebug() << "pdir : "<< parent_dir;
#endif
    QDir dDir(parent_dir);    //�¼�Ŀ¼
    QDir dFile(parent_dir);   //�¼��ļ�

    //����¼��ļ�
    dFile.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dFile.setSorting(QDir::Size | QDir::Reversed);

    //��ǰ�����б�
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

        //��δ���ӣ�����Ӽ���
        if(!watch_files.contains(file_path)){
            qDebug() << CStr2LocalQStr("��Ӽ����ļ���") << file_path;
            m_pFilesysWatcher->addPath(file_path);        //��Ӽ���
            //���˸����ص��ļ�
            if(file_path != m_temp_path){
                //## ͬ���ļ�
                SyncFileOrDir(file_path, SYNC_UPFILE);
            }
            else {
                qDebug() << CStr2LocalQStr("�����ļ�") << m_temp_path;
                m_temp_path = "";
            }

        }
    }

    //����¼�Ŀ¼
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
        updateFolderChilds(pChild, dir_path);       //����Ŀ¼

        //��δ���ӣ�����Ӽ���
        if(!watch_dirs.contains(dir_path)){
            qDebug() << CStr2LocalQStr("��Ӽ���Ŀ¼��") << dir_path;
            m_pFilesysWatcher->addPath(dir_path);        //��Ӽ���
            //## ͬ�����½�Ŀ¼
            SyncFileOrDir(dir_path, SYNC_MKDIR);
        }
    }

}
