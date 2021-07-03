#ifndef FILE_COMP_API
#define FILE_COMP_API

#include <string>
#include <vector>

const std::string STORE_DIR_NAME = ".storeFile/";
const std::string STORE_PART_NAME = STORE_DIR_NAME + ".fileRecordStore";
const int MAX_PATH_SIZE = 256;

struct FileRecord
{
	char partPath[MAX_PATH_SIZE];
	struct _stat fileStat;
	char fileMD5[33];
};

struct UpdateResult
{
	std::string partPath;
	int todo;	/* ��ʱ0--�ϴ���1--����  2--���ظ��� */
	bool isDir;
	bool nameConflict;
	std::string newPath;	//ֻ��todoΪ���ظ���ʱ��������Ч�ģ�
};

// �������ʱд�أ����綪ʧû���ü�д��ȥ���´λ���Ϊstat�����仯��ִ��upFile�����ϴ�
// ������ļ�û�ģ������û���ͬ·���ļ���Զ���߼���Ϊ�ޱ仯�������봫��������������
// ���ԣ�����һֱд��

class FileCompare 
{
public:
	std::vector<FileRecord> recordlist;
	std::string rootPath;	//�����·�����������Ϊ����ͬ��Ŀ¼
	bool syncFirst;

	bool initListBySearch();	//����ͬ��Ŀ¼�����������¼���Ŀ¼д��recordlist

	std::vector<std::string> remoteFileList;
	std::vector<std::string> remoteMD5List;
	std::vector<std::string> remotePathList;

	bool pathExists(std::string partpath);
	std::string getMD5ByPath(std::string partpath);
public:
	FileCompare();

	bool loadFromConf(unsigned int userid, std::string filename = STORE_PART_NAME);
	bool writeAllToConf(unsigned int userid, std::string filename = STORE_PART_NAME);

	bool setSyncFirst(bool state);
	bool addFileRecord(std::string partpath);	//û������ӣ����򸲸�
	bool delFileRecord(std::string partpath);
	bool setLocalSyncPath(std::string syncpath);
	bool setRemoteAllFile(std::vector<std::string> filelist, std::vector<std::string> md5list, std::vector<std::string> pathlist);

	std::vector<UpdateResult> getResult();

public:
	static const int UPFILE = 0;
	static const int DOWNFILE = 1;
	static const int LOCALNAMECHANGE = 2;
};

// ���й��ܺ���
void getfileall(std::string path, std::vector<std::string> &dirpath);
#endif
