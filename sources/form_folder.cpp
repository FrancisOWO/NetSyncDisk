#include "form_folder.h"
#include "ui_form_folder.h"

#include <QStringList>
#include <QMessageBox>
#include <QCheckBox>

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFileDialog>

#include <QDebug>

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
    connect(ui->pbtnChooseRoot, SIGNAL(clicked()), this, SLOT(chooseRootDir()));
    connect(ui->pbtnChgRoot, SIGNAL(clicked()), this, SLOT(changeRootDir()));
    connect(ui->pbtnUpdFolder, SIGNAL(clicked()), this, SLOT(updateFolderTree()));

    connect(ui->pbtnChooseFile, SIGNAL(clicked()), this, SLOT(chooseFile()));

    connect(ui->chkAutoUpd, SIGNAL(clicked()), this, SLOT(updateAutoUpdFlag()));

    //����Ŀ¼���ļ�
    connect(m_pFilesysWatcher, SIGNAL(fileChanged(const QString &)),
            this, SLOT(onFileChanged(const QString &)));
    connect(m_pFilesysWatcher, SIGNAL(directoryChanged(const QString &)),
            this, SLOT(onDirChanged(const QString &)));

}

void FormFolder::chooseRootDir()
{
    QString path = QFileDialog::getExistingDirectory(
                nullptr, CStr2LocalQStr("ѡ�񶥲�Ŀ¼"), m_root_dir);
    ui->lnRootDir->setText(path);
}

void FormFolder::changeRootDir()
{
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
        QString title = CStr2LocalQStr("��ʾ");
        QString info = path + CStr2LocalQStr("Ŀ¼�����ɹ���");
        QMessageBox::information(nullptr, title, info);
        //Ŀ¼����
        m_pFilesysWatcher->removePath(m_root_dir);  //ɾ��ԭĿ¼
        m_root_dir = path;
        m_pFilesysWatcher->addPath(m_root_dir);     //�����Ŀ¼
        qDebug() << CStr2LocalQStr("����Ŀ¼") << m_root_dir;
        InitFolderTree();   //����Ŀ¼��
    }
    ui->lnRootDir->setText(m_root_dir);
}

void FormFolder::setRootDir(const QString& dir)
{
    m_root_dir = dir;
    ui->lnRootDir->setText(m_root_dir);
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
        int start = m_root_dir.length() + 1;    //��ȡ���·��
        emit rmfile(path.mid(start));

        //ˢ��Ŀ¼��
        if(m_autoUpd_flag)      //�Զ�ˢ��
            updateFolderTree();
    }
    else {
        qDebug() << CStr2LocalQStr("�޸��ļ���") << path;
        //�޸ĺ󣬱�Ϊ����ɾ�����ǿ����ϴ�
        int file_len = fileInfo.size();
        int start = m_root_dir.length() + 1;    //��ȡ���·��
        QString relt_path = path.mid(start);
        if(file_len != 0){
            //## ͬ�����ϴ��ļ�
            emit upfile(relt_path);
        }
        else{
            //## ͬ����ɾ���ļ�
            emit rmfile(relt_path);
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
        int start = m_root_dir.length() + 1;    //��ȡ���·��
        emit rmdir(path.mid(start) + "/");
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
    ui->lnFilePath->setText(file_path);
    emit upfile(file_path);
}

void FormFolder::updateAutoUpdFlag()
{
    m_autoUpd_flag = ui->chkAutoUpd->isChecked();
    //qDebug() << "auto flag: "<< m_autoUpd_flag;
    if(m_autoUpd_flag){         //ˢ��Ŀ¼
        updateFolderTree();
    }
}

void FormFolder::SyncFile(const QString &file_path)
{
    QFileInfo file_info(file_path);
    if(file_info.size() == 0){
        qDebug() << CStr2LocalQStr("���ļ���ͬ����");
        return;
    }
    //ͬ���ļ�
    SyncFile(file_path);
    /*
    int base_len = m_root_dir.length() + 1;
    QString rel_path = file_path.mid(base_len);
    emit upfile(rel_path);
    */
}

void FormFolder::SyncDir(const QString &dir_path)
{
    //ͬ��Ŀ¼
    int base_len = m_root_dir.length() + 1;
    QString rel_path = dir_path.mid(base_len) + "/";
    emit mkdir(rel_path);
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
        //ͬ��Ŀ¼

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
            //## ���ļ���ͬ��
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
            SyncDir(dir_path);
            /*
            int start = m_root_dir.length() + 1;    //��ȡ���·��
            emit mkdir(dir_path.mid(start) + "/");
            */
        }
    }

}
