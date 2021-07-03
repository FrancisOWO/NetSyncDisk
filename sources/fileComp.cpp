#define _CRT_SECURE_NO_WARNINGS
#include <io.h>
#include <direct.h>
#include <set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "fileComp.h"
#include "md5.h"

using namespace std;

bool FileCompare::initListBySearch()
{
	vector<string> dirpath;
	getfileall(this->rootPath, dirpath);
	dirpath.erase(dirpath.begin());

	// 依然要随时判断目录是否存在
	if (0 != _access(this->rootPath.data(), 0))
		return false;

	MD5 md5;

	for (auto it = dirpath.begin(); it != dirpath.end(); ++it) {
		FileRecord fr;
		// 文件路径很显然一定是合理字符串
		bool have_ = rootPath.at(rootPath.size() - 1) == '/';
		strcpy_s(fr.partPath, it->substr(rootPath.size() + !have_).data());
		
		// 如果当前文件路径是目录，则stat和md5直接初始化全0
		if (it->at(it->size() - 1) == '\\') {
			memset(&fr.fileStat, 0, sizeof(fr.fileStat));
			memset(fr.fileMD5, 0, sizeof(fr.fileMD5));
		}
		else {// 否则获取真实stat和md5并赋值
			_stat(it->data(), &fr.fileStat);

			ifstream in(it->data(), ios::binary);
			md5.reset();
			md5.update(in);
			strcpy_s(fr.fileMD5, md5.toString().data());
			in.close();
		}
		recordlist.push_back(fr);
	}
	return true;
}

bool FileCompare::pathExists(std::string partpath)
{
	for (auto it = recordlist.begin(); it != recordlist.end(); ++it) {
		if (partpath._Equal(it->partPath))
			return true;
	}

	return false;
}

std::string FileCompare::getMD5ByPath(std::string partpath)
{
	for (auto it = recordlist.begin(); it != recordlist.end(); ++it) {
		if (partpath._Equal(it->partPath)) 
			return string(it->fileMD5);
	}

	return string();
}

FileCompare::FileCompare()
{
	this->syncFirst = true;
}

bool FileCompare::loadFromConf(unsigned int userid, std::string filename)
{
	// 如果目录不存在，说明本客户机第一次运行，需要创建该目录
	if (0 != _access(STORE_DIR_NAME.data(), 0))
		_mkdir(STORE_DIR_NAME.data());

	// 拼装用户文件结构
	stringstream ss;
	ss << userid;
	string USER_PART1 = "_";
	string USER_PART2;
	ss >> USER_PART2;
	
	string FULL_PATH = filename + USER_PART1 + USER_PART2;
	// 如果文件不存在，说明本用户第一次同步，需要创建该文件
	FILE* fd;
	if (0 != _access(FULL_PATH.data(), 0)) {
		fd = fopen(FULL_PATH.data(), "wb");
		fclose(fd);
	}

	// 如果是初始同步，不从文件读取，而是直接计算得出，并写入文件
	if (this->syncFirst) {
		initListBySearch();
		writeAllToConf(userid, filename);
	}
	else {
		struct _stat toSize;
		_stat(FULL_PATH.data(), &toSize);
		int num = toSize.st_size / (sizeof(FileRecord::partPath) + sizeof(FileRecord::fileStat) + sizeof(FileRecord::fileMD5));

		// 否则根据文件内容重置
		FILE *fd;
		fd = fopen(FULL_PATH.data(), "rb");
		recordlist.clear();
		for (int i = 0; i < num; i++) {
			FileRecord fr;
			fread(fr.partPath, 1, sizeof(fr.partPath), fd);
			fread(&fr.fileStat, 1, sizeof(fr.fileStat), fd);
			fread(fr.fileMD5, 1, sizeof(fr.fileMD5), fd);
			recordlist.push_back(fr);
		}
		fclose(fd);
	}

	return false;
}

bool FileCompare::writeAllToConf(unsigned int userid, std::string filename)
{
	// 如果目录不存在，说明本客户机第一次运行，需要创建该目录
	if (0 != _access(STORE_DIR_NAME.data(), 0))
		_mkdir(STORE_DIR_NAME.data());


	// 拼装用户文件结构
	stringstream ss;
	ss << userid;
	string USER_PART1 = "_";
	string USER_PART2;
	ss >> USER_PART2;

	string FULL_PATH = filename + USER_PART1 + USER_PART2;
	
	FILE *fd;
	fd = fopen(FULL_PATH.data(), "wb");

	for (auto it = recordlist.begin(); it != recordlist.end(); ++it) {
		fwrite(it->partPath, 1, sizeof(it->partPath), fd);
		fwrite(&it->fileStat, 1, sizeof(it->fileStat), fd);
		fwrite(it->fileMD5, 1, sizeof(it->fileMD5), fd);
	}

	fclose(fd);
	return true;
}

bool FileCompare::setSyncFirst(bool state)
{
	syncFirst = state;
	return true;
}

bool FileCompare::addFileRecord(std::string partpath)
{
	// 如果找到则对文件记录进行修改
	auto it = recordlist.begin();
	for (; it != recordlist.end(); ++it)
		if (partpath._Equal(it->partPath) && partpath.at(partpath.size() - 1) != '\\') {
			_stat((rootPath + "\\" + partpath).data(), &it->fileStat);

			ifstream in((rootPath + "\\" + partpath).data(), ios::binary);
			MD5 md5;
			md5.reset();
			md5.update(in);
			strcpy_s(it->fileMD5, md5.toString().data());
			in.close();

			break;
		}

	// 如果找不到则对文件记录进行添加
	if (it == recordlist.end()) {
		FileRecord fr;
		// 文件路径很显然一定是合理字符串
		strcpy_s(fr.partPath, partpath.data());

		// 如果当前文件路径是目录，则stat和md5直接初始化全0
		if (partpath.at(partpath.size() - 1) == '\\') {
			memset(&fr.fileStat, 0, sizeof(fr.fileStat));
			memset(fr.fileMD5, 0, sizeof(fr.fileMD5));
		}
		else {// 否则获取真实stat和md5并赋值
			_stat((rootPath + "\\" + partpath).data(), &fr.fileStat);

			ifstream in((rootPath + "\\" + partpath).data(), ios::binary);
			MD5 md5;
			md5.reset();
			md5.update(in);
			strcpy_s(fr.fileMD5, md5.toString().data());
			in.close();
		}
		recordlist.push_back(fr);
	}

	return true;
}

bool FileCompare::delFileRecord(std::string partpath)
{
	auto it = recordlist.begin();
	for (; it != recordlist.end(); ++it)
		if (partpath._Equal(it->partPath)) {
			recordlist.erase(it);
			return true;
		}
	return false;
}

bool FileCompare::setLocalSyncPath(std::string syncpath)
{
	if (0 != _access(syncpath.data(), 0))
		return false;

	this->rootPath = syncpath;
	return true;
}

bool FileCompare::setRemoteAllFile(std::vector<std::string> filelist, std::vector<std::string> md5list, std::vector<std::string> pathlist)
{
	remoteFileList = filelist;
	remoteMD5List = md5list;
	remotePathList = pathlist;

	return true;
}

std::vector<UpdateResult> FileCompare::getResult()
{
	vector<UpdateResult> res;

	// 生成所有目录结果
	// 对所有本地存在但远端不存在的目录请求上传
	for (auto it = recordlist.begin(); it != recordlist.end(); ++it) {
		if (*string(it->partPath).rbegin() == '\\' && find(remotePathList.begin(), remotePathList.end(), string(it->partPath)) == remotePathList.end()) {
			UpdateResult ur;
			ur.partPath = it->partPath;
			ur.todo = FileCompare::UPFILE;
			ur.isDir = true;
			ur.nameConflict = false;

			res.push_back(ur);
		}
	}
	// 对所有远端存在但本地不存在的目录请求下载
	for (auto it = remotePathList.begin(); it != remotePathList.end(); ++it) {
		if (!this->pathExists(*it)) {
			UpdateResult ur;
			ur.partPath = *it;
			ur.todo = FileCompare::DOWNFILE;
			ur.isDir = true;
			ur.nameConflict = false;

			res.push_back(ur);
		}
	}

	// 对所有本地文件做检测，逻辑规约：
	if (this->syncFirst) {	//如果是初次连接
		// 所有本地文件都做一次upfile
		for (auto it = recordlist.begin(); it != recordlist.end(); ++it) {
			//目录直接无视
			if (*string(it->partPath).rbegin() == '\\')
				continue;
			auto md5it = find(remoteMD5List.begin(), remoteMD5List.end(), string(it->fileMD5));
			if (md5it != remoteMD5List.end()) {	//是文件且远端存在
				// 文件名一致，不做处理
				if (remoteFileList.at(md5it - remoteMD5List.begin()) != it->partPath) {
					UpdateResult ur;
					ur.partPath = it->partPath;
					ur.todo = FileCompare::UPFILE;
					ur.isDir = false;
					ur.nameConflict = false;
					res.push_back(ur);
				}
			}
			else if (strlen(it->fileMD5) != 0) {	//是文件但远端不存在
				UpdateResult ur;
				ur.partPath = it->partPath;
				ur.todo = FileCompare::UPFILE;
				ur.isDir = false;
				ur.nameConflict = find(remoteFileList.begin(), remoteFileList.end(), string(it->partPath)) != remoteFileList.end();
				res.push_back(ur);
			}
		}
		// 对远端文件
		for (auto it = remoteFileList.begin(); it != remoteFileList.end(); ++it) {
			if (!pathExists(*it)) {	//只要文件不存在则请求下载
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = false;
				res.push_back(ur);
			}
			else if (getMD5ByPath(*it) != remoteMD5List.at(it - remoteFileList.begin())) {
				//若文件名相同但md5不同，则要下载到本地
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = true;
				res.push_back(ur);
			}
		}
	}
	else {	//如果不是初次连接
		// 首先扫描装载好的本地记录
		for (unsigned int i = 0; i < recordlist.size(); ++i) {
			string fullFilePath = rootPath + "\\" + recordlist.at(i).partPath;
		
			// 找不到路径对应文件，删除记录
			if (0 != _access(fullFilePath.data(), 0)) {
				recordlist.erase(recordlist.begin() + i);
				continue;
			}

			//目录直接无视
			if (*string(recordlist.at(i).partPath).rbegin() == '\\')
				continue;

			// 本文件stat无变化，不上传
			struct _stat STAT;
			_stat(fullFilePath.data(), &STAT);
			if (STAT.st_mtime == recordlist.at(i).fileStat.st_mtime)
				continue;

			// 否则发送上传请求
			UpdateResult ur;
			ur.partPath = recordlist.at(i).partPath;
			ur.todo = FileCompare::UPFILE;
			ur.isDir = false;
			ur.nameConflict = false;
			res.push_back(ur);
		}


		// 接下来扫描所有本地文件
		//
		set<string> toChangeName;
		MD5 md5;
		vector<string> dirpath;
		getfileall(this->rootPath, dirpath);
		dirpath.erase(dirpath.begin());
		for (auto it = dirpath.begin(); it != dirpath.end(); ++it) {
			//目录直接无视
			if (it->at(it->size() - 1) == '\\')
				continue;

			// 所有存在的，都被初次扫描处理过，这里只处理新增的
			bool have_ = rootPath.at(rootPath.size() - 1) == '/';
			if (!pathExists(it->substr(rootPath.size() + !have_))) {
				FileRecord fr;

				strcpy_s(fr.partPath, it->substr(rootPath.size() + !have_).data());

				// 如果当前文件路径是目录，则stat和md5直接初始化全0
				if (it->at(it->size() - 1) == '\\') {
					memset(&fr.fileStat, 0, sizeof(fr.fileStat));
					memset(fr.fileMD5, 0, sizeof(fr.fileMD5));
				}
				else {
					_stat(it->data(), &fr.fileStat);

					ifstream in(it->data(), ios::binary);
					md5.reset();
					md5.update(in);
					strcpy_s(fr.fileMD5, md5.toString().data());
					in.close();
				}

				recordlist.push_back(fr);

				// 如果是新增目录，直接同步到远端
				if (it->at(it->size() - 1) == '\\') {
					UpdateResult ur;
					ur.partPath = fr.partPath;
					ur.todo = FileCompare::UPFILE;
					ur.isDir = true;
					ur.nameConflict = false;

					res.push_back(ur);
					continue;
				}

				// 如果是新增的文件，需要复杂处理
				// 首先根据md5和远端对比，如果远端不存在MD5，则上传/改名上传
				auto MD5it = find(remoteMD5List.begin(), remoteMD5List.end(), string(fr.fileMD5));
				if (MD5it == remoteMD5List.end()) {
					UpdateResult ur;
					ur.partPath = fr.partPath;
					ur.todo = FileCompare::UPFILE;
					ur.isDir = false;
					ur.nameConflict = find(remoteFileList.begin(), remoteFileList.end(), ur.partPath) != remoteFileList.end();

					res.push_back(ur);
				}
				else if (!remoteFileList.at(MD5it - remoteMD5List.begin())._Equal(fr.partPath)) {
					UpdateResult ur;
					ur.partPath = fr.partPath;
					ur.todo = FileCompare::LOCALNAMECHANGE;
					ur.isDir = false;
					ur.nameConflict = false;
					ur.newPath = remoteFileList.at(MD5it - remoteMD5List.begin());
					res.push_back(ur);

					// 如果要改名，需要记录，否则还是会产生远端下载请求
					toChangeName.insert(ur.newPath);
				}
			}
		}

		// 最后扫描所有远端目录
		for (auto it = remoteFileList.begin(); it != remoteFileList.end(); ++it) {
			if (!pathExists(*it) && toChangeName.find(*it) == toChangeName.end()) {	//只要文件不存在则请求下载
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = false;
				res.push_back(ur);
			}
			else if (pathExists(*it) && getMD5ByPath(*it) != remoteMD5List.at(it - remoteFileList.begin())) {
				//若文件名相同但md5不同，则要下载到本地
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = true;
				res.push_back(ur);
			}
		}
	}

	return res;
}

// 获取某目录下所有目录及文件的功能函数 
void getfileall(string path, vector<string> &dirpath) 
{
	if (0 != _access(path.data(), 0)) {
		dirpath.push_back("");
		return;
	}

	dirpath.push_back(path + "\\");
	// _finddata_t是一个结构体，要用到#include <io.h>头文件；
	struct _finddata_t fileinfo;
	long ld;
	if ((ld = _findfirst((path + "\\*").c_str(), &fileinfo)) != -1l) {
		do {
			if ((fileinfo.attrib&_A_SUBDIR)) {  //如果是文件夹；
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {  //如果文件名不是.或者,,,则递归获取子文件中的文件；
					getfileall(path + "\\" + fileinfo.name, dirpath);  //递归子文件夹；
				}
			}
			else   //如果是文件；
			{
				dirpath.push_back(path + "\\" + fileinfo.name);
			}
		} while (_findnext(ld, &fileinfo) == 0);
		_findclose(ld);
	}
}