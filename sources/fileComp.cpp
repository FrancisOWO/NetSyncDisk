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

	// ��ȻҪ��ʱ�ж�Ŀ¼�Ƿ����
	if (0 != _access(this->rootPath.data(), 0))
		return false;

	MD5 md5;

	for (auto it = dirpath.begin(); it != dirpath.end(); ++it) {
		FileRecord fr;
		// �ļ�·������Ȼһ���Ǻ����ַ���
		bool have_ = rootPath.at(rootPath.size() - 1) == '/';
		strcpy_s(fr.partPath, it->substr(rootPath.size() + !have_).data());
		
		// �����ǰ�ļ�·����Ŀ¼����stat��md5ֱ�ӳ�ʼ��ȫ0
		if (it->at(it->size() - 1) == '\\') {
			memset(&fr.fileStat, 0, sizeof(fr.fileStat));
			memset(fr.fileMD5, 0, sizeof(fr.fileMD5));
		}
		else {// �����ȡ��ʵstat��md5����ֵ
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
	// ���Ŀ¼�����ڣ�˵�����ͻ�����һ�����У���Ҫ������Ŀ¼
	if (0 != _access(STORE_DIR_NAME.data(), 0))
		_mkdir(STORE_DIR_NAME.data());

	// ƴװ�û��ļ��ṹ
	stringstream ss;
	ss << userid;
	string USER_PART1 = "_";
	string USER_PART2;
	ss >> USER_PART2;
	
	string FULL_PATH = filename + USER_PART1 + USER_PART2;
	// ����ļ������ڣ�˵�����û���һ��ͬ������Ҫ�������ļ�
	FILE* fd;
	if (0 != _access(FULL_PATH.data(), 0)) {
		fd = fopen(FULL_PATH.data(), "wb");
		fclose(fd);
	}

	// ����ǳ�ʼͬ���������ļ���ȡ������ֱ�Ӽ���ó�����д���ļ�
	if (this->syncFirst) {
		initListBySearch();
		writeAllToConf(userid, filename);
	}
	else {
		struct _stat toSize;
		_stat(FULL_PATH.data(), &toSize);
		int num = toSize.st_size / (sizeof(FileRecord::partPath) + sizeof(FileRecord::fileStat) + sizeof(FileRecord::fileMD5));

		// ��������ļ���������
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
	// ���Ŀ¼�����ڣ�˵�����ͻ�����һ�����У���Ҫ������Ŀ¼
	if (0 != _access(STORE_DIR_NAME.data(), 0))
		_mkdir(STORE_DIR_NAME.data());


	// ƴװ�û��ļ��ṹ
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
	// ����ҵ�����ļ���¼�����޸�
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

	// ����Ҳ�������ļ���¼�������
	if (it == recordlist.end()) {
		FileRecord fr;
		// �ļ�·������Ȼһ���Ǻ����ַ���
		strcpy_s(fr.partPath, partpath.data());

		// �����ǰ�ļ�·����Ŀ¼����stat��md5ֱ�ӳ�ʼ��ȫ0
		if (partpath.at(partpath.size() - 1) == '\\') {
			memset(&fr.fileStat, 0, sizeof(fr.fileStat));
			memset(fr.fileMD5, 0, sizeof(fr.fileMD5));
		}
		else {// �����ȡ��ʵstat��md5����ֵ
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

	// ��������Ŀ¼���
	// �����б��ش��ڵ�Զ�˲����ڵ�Ŀ¼�����ϴ�
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
	// ������Զ�˴��ڵ����ز����ڵ�Ŀ¼��������
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

	// �����б����ļ�����⣬�߼���Լ��
	if (this->syncFirst) {	//����ǳ�������
		// ���б����ļ�����һ��upfile
		for (auto it = recordlist.begin(); it != recordlist.end(); ++it) {
			//Ŀ¼ֱ������
			if (*string(it->partPath).rbegin() == '\\')
				continue;
			auto md5it = find(remoteMD5List.begin(), remoteMD5List.end(), string(it->fileMD5));
			if (md5it != remoteMD5List.end()) {	//���ļ���Զ�˴���
				// �ļ���һ�£���������
				if (remoteFileList.at(md5it - remoteMD5List.begin()) != it->partPath) {
					UpdateResult ur;
					ur.partPath = it->partPath;
					ur.todo = FileCompare::UPFILE;
					ur.isDir = false;
					ur.nameConflict = false;
					res.push_back(ur);
				}
			}
			else if (strlen(it->fileMD5) != 0) {	//���ļ���Զ�˲�����
				UpdateResult ur;
				ur.partPath = it->partPath;
				ur.todo = FileCompare::UPFILE;
				ur.isDir = false;
				ur.nameConflict = find(remoteFileList.begin(), remoteFileList.end(), string(it->partPath)) != remoteFileList.end();
				res.push_back(ur);
			}
		}
		// ��Զ���ļ�
		for (auto it = remoteFileList.begin(); it != remoteFileList.end(); ++it) {
			if (!pathExists(*it)) {	//ֻҪ�ļ�����������������
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = false;
				res.push_back(ur);
			}
			else if (getMD5ByPath(*it) != remoteMD5List.at(it - remoteFileList.begin())) {
				//���ļ�����ͬ��md5��ͬ����Ҫ���ص�����
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = true;
				res.push_back(ur);
			}
		}
	}
	else {	//������ǳ�������
		// ����ɨ��װ�غõı��ؼ�¼
		for (unsigned int i = 0; i < recordlist.size(); ++i) {
			string fullFilePath = rootPath + "\\" + recordlist.at(i).partPath;
		
			// �Ҳ���·����Ӧ�ļ���ɾ����¼
			if (0 != _access(fullFilePath.data(), 0)) {
				recordlist.erase(recordlist.begin() + i);
				continue;
			}

			//Ŀ¼ֱ������
			if (*string(recordlist.at(i).partPath).rbegin() == '\\')
				continue;

			// ���ļ�stat�ޱ仯�����ϴ�
			struct _stat STAT;
			_stat(fullFilePath.data(), &STAT);
			if (STAT.st_mtime == recordlist.at(i).fileStat.st_mtime)
				continue;

			// �������ϴ�����
			UpdateResult ur;
			ur.partPath = recordlist.at(i).partPath;
			ur.todo = FileCompare::UPFILE;
			ur.isDir = false;
			ur.nameConflict = false;
			res.push_back(ur);
		}


		// ������ɨ�����б����ļ�
		//
		set<string> toChangeName;
		MD5 md5;
		vector<string> dirpath;
		getfileall(this->rootPath, dirpath);
		dirpath.erase(dirpath.begin());
		for (auto it = dirpath.begin(); it != dirpath.end(); ++it) {
			//Ŀ¼ֱ������
			if (it->at(it->size() - 1) == '\\')
				continue;

			// ���д��ڵģ���������ɨ�账���������ֻ����������
			bool have_ = rootPath.at(rootPath.size() - 1) == '/';
			if (!pathExists(it->substr(rootPath.size() + !have_))) {
				FileRecord fr;

				strcpy_s(fr.partPath, it->substr(rootPath.size() + !have_).data());

				// �����ǰ�ļ�·����Ŀ¼����stat��md5ֱ�ӳ�ʼ��ȫ0
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

				// ���������Ŀ¼��ֱ��ͬ����Զ��
				if (it->at(it->size() - 1) == '\\') {
					UpdateResult ur;
					ur.partPath = fr.partPath;
					ur.todo = FileCompare::UPFILE;
					ur.isDir = true;
					ur.nameConflict = false;

					res.push_back(ur);
					continue;
				}

				// ������������ļ�����Ҫ���Ӵ���
				// ���ȸ���md5��Զ�˶Աȣ����Զ�˲�����MD5�����ϴ�/�����ϴ�
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

					// ���Ҫ��������Ҫ��¼�������ǻ����Զ����������
					toChangeName.insert(ur.newPath);
				}
			}
		}

		// ���ɨ������Զ��Ŀ¼
		for (auto it = remoteFileList.begin(); it != remoteFileList.end(); ++it) {
			if (!pathExists(*it) && toChangeName.find(*it) == toChangeName.end()) {	//ֻҪ�ļ�����������������
				UpdateResult ur;
				ur.partPath = *it;
				ur.todo = FileCompare::DOWNFILE;
				ur.isDir = false;
				ur.nameConflict = false;
				res.push_back(ur);
			}
			else if (pathExists(*it) && getMD5ByPath(*it) != remoteMD5List.at(it - remoteFileList.begin())) {
				//���ļ�����ͬ��md5��ͬ����Ҫ���ص�����
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

// ��ȡĳĿ¼������Ŀ¼���ļ��Ĺ��ܺ��� 
void getfileall(string path, vector<string> &dirpath) 
{
	if (0 != _access(path.data(), 0)) {
		dirpath.push_back("");
		return;
	}

	dirpath.push_back(path + "\\");
	// _finddata_t��һ���ṹ�壬Ҫ�õ�#include <io.h>ͷ�ļ���
	struct _finddata_t fileinfo;
	long ld;
	if ((ld = _findfirst((path + "\\*").c_str(), &fileinfo)) != -1l) {
		do {
			if ((fileinfo.attrib&_A_SUBDIR)) {  //������ļ��У�
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {  //����ļ�������.����,,,��ݹ��ȡ���ļ��е��ļ���
					getfileall(path + "\\" + fileinfo.name, dirpath);  //�ݹ����ļ��У�
				}
			}
			else   //������ļ���
			{
				dirpath.push_back(path + "\\" + fileinfo.name);
			}
		} while (_findnext(ld, &fileinfo) == 0);
		_findclose(ld);
	}
}