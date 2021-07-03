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
	int todo;	/* 暂时0--上传，1--下载  2--本地改名 */
	bool isDir;
	bool nameConflict;
	std::string newPath;	//只有todo为本地改名时，才是有效的！
};

// 如果不及时写回，掉电丢失没来得及写回去，下次会认为stat发生变化，执行upFile请求上传
// 但如果文件没改，对于用户的同路径文件，远端逻辑认为无变化，返回秒传，不做其它处理
// 所以，不用一直写回

class FileCompare 
{
public:
	std::vector<FileRecord> recordlist;
	std::string rootPath;	//传入根路径，本情况下为本地同步目录
	bool syncFirst;

	bool initListBySearch();	//根据同步目录，将其所有下级子目录写入recordlist

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
	bool addFileRecord(std::string partpath);	//没有则添加，有则覆盖
	bool delFileRecord(std::string partpath);
	bool setLocalSyncPath(std::string syncpath);
	bool setRemoteAllFile(std::vector<std::string> filelist, std::vector<std::string> md5list, std::vector<std::string> pathlist);

	std::vector<UpdateResult> getResult();

public:
	static const int UPFILE = 0;
	static const int DOWNFILE = 1;
	static const int LOCALNAMECHANGE = 2;
};

// 所有功能函数
void getfileall(std::string path, std::vector<std::string> &dirpath);
#endif
