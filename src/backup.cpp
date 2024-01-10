#include "client.h"
#include "server.h"
#include <iostream>
#include <filesystem>
#include<stdlib.h>
#include<thread>
#include<vector>
#include<sstream>
#include<direct.h>


#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include<windows.h>

#pragma comment(lib, "Ws2_32.lib")

namespace fs = std::filesystem;

using namespace std;

void writeLog(const std::string& content) {
    std::ofstream outputFile(getExePath()+"/log", std::ios::app);
    if (outputFile) {
        outputFile << content <<std::endl;
        std::cout << "Record written successfully." << std::endl;
    } 
}
 
/*
string timeNow(){
    // ??????????
    auto now = std::chrono::system_clock::now();
    // ???? time_t ?????????????
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
*/

bool checkFileSame(const fs::path& filePath_root, const fs::path& savePath_root,const fs::path& filePath, const fs::path& savePath){
    fs::path p1 = fs::relative(filePath, filePath_root);
    fs::path p2 = fs::relative(savePath, savePath_root);
    if(p1==p2&&fs::exists(savePath)){
        return true;
    }else{
        return false;
    }
}

void copyFileData(const fs::path& filePath, const fs::path& savePath) {
    std::ifstream inputFile(filePath, std::ios::binary);
    std::ofstream outputFile(savePath, std::ios::binary);

    if (inputFile && outputFile) {
        outputFile << inputFile.rdbuf();
        std::cout<<filePath.filename().string()<< "  File data saved successfully." << std::endl;
    } else {
        std::cout <<filePath.filename().string()<< "  Failed to save file data." << std::endl;
    }
}

void copyDirectoryData(const fs::path& directoryPath, const fs::path& saveDirectory_root,bool writeLogFlag) {
    if(fs::is_regular_file(directoryPath)){
        copyFileData(directoryPath,saveDirectory_root/directoryPath.filename());
    }
    fs::path saveDirectory = saveDirectory_root.string()+"/"+directoryPath.filename().string();
    if (!fs::exists(saveDirectory)) {
        fs::create_directories(saveDirectory);
    }
    else{
        cout<<saveDirectory.filename().string()<<" -- already exists"<<endl;
    }
    if(fs::is_directory(directoryPath)){

    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            fs::path relativePath = fs::relative(entry.path(), directoryPath);
            fs::path savePath = saveDirectory / relativePath;
             //?????????????

            if(checkFileSame(directoryPath,saveDirectory,entry.path(), savePath)){
                std::cout<<savePath.string()<<" -- already exists , this file is replaced"<<endl;
            };
            copyFileData(entry.path(), savePath);
        }
        else if (entry.is_directory()){
            
            fs::path relativePath = fs::relative(entry.path(), directoryPath);
            fs::path child_dir = saveDirectory / relativePath;
            if (!fs::exists(child_dir)) {
                fs::create_directories(child_dir);
            }
        }
    }

    }

    if(writeLogFlag){
    string record=directoryPath.filename().string()+" "+directoryPath.string()+" "+saveDirectory.string()+" "+timeNow();
    writeLog(record);
    }
}

void showLog(){
    std::ifstream file(getExePath()+"/log");
    std::string record;
    int i=-1;
    while(std::getline(file, record)){
        std::cout<<++i<<": "<<record<<std::endl;
    }
}

void restoreFile(){
    std::ifstream file(getExePath()+"/log");
    std::string record;
    int i=-1;
    while(std::getline(file, record)){
        std::cout<<++i<<": "<<record<<std::endl;
    }
    std::cout<<"-------------------------"<<std::endl;
    std::cout<<"Please enter the number of the record you want to restore: ";
    int num;
    std::cin>>num;
    if(num<1||num>i){
        std::cout<<"invalid input"<<std::endl;
        return;
    }
    std::cout<<"-------------------------"<<std::endl;
    std::ifstream file2(getExePath()+"/log");
    i=-1;
    while(std::getline(file2, record)){
        if(++i==num){
            break;
        }
    }
    std::stringstream ss(record);
    std::string name;
    fs::path path;
    fs::path savePath;
    std::string time;
    ss>>name>>path>>savePath>>time;
    std::cout<<"choosen data is restoring..."<<std::endl;

    copyDirectoryData(savePath,path.parent_path(),false);

    std::cout<<"restore successfully"<<std::endl;
}

void xorEncryptFile(const std::string& filePath, const std::string& key) {
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile) {
        std::cout << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::ofstream outputFile(filePath + ".encrypted", std::ios::binary);
    if (!outputFile) {
        std::cout << "Failed to create encrypted file." << std::endl;
        return;
    }

    char byte;
    size_t keyIndex = 0;
    while (inputFile.get(byte)) {
        byte ^= key[keyIndex % key.size()];
        outputFile.put(byte);
        ++keyIndex;
    }
    inputFile.close();
    outputFile.close();
    fs::path file_deleted = filePath;
    fs::remove(file_deleted);
    std::cout << "File encrypted successfully." << std::endl;
}

void xorDecryptFile(const std::string& filePath_without_en, const std::string& key) {
    std::string filePath=filePath_without_en+".encrypted";
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile) {
        std::cout << "Failed to open file: " << filePath << std::endl;
        return;
    }
    std::string decrypt = filePath.substr(0,filePath.find_last_of('.'));
    std::ofstream outputFile(decrypt, std::ios::binary);
    if (!outputFile) {
        std::cout << "Failed to create decrypted file." << std::endl;
        return;
    }

    char byte;
    size_t keyIndex = 0;
    while (inputFile.get(byte)) {
        byte ^= key[keyIndex % key.size()];
        outputFile.put(byte);
        ++keyIndex;
    }
    inputFile.close();
    outputFile.close();
    fs::path file_deleted = filePath;
    fs::remove(file_deleted);
    std::cout << "File decrypted successfully." << std::endl;
}

void encryptDirectoryData(const fs::path& directoryPath, const std::string& key) {
    if(fs::is_regular_file(directoryPath)){
        xorEncryptFile(directoryPath.string(),key);
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            xorEncryptFile(entry.path().string(), key);
        }
    }
}

void decryptDirectoryData(const fs::path& directoryPath, const std::string& key) {
    fs::path encrypted = directoryPath.string()+".encrypted";
    if(fs::is_regular_file(encrypted)){
        xorDecryptFile(directoryPath.string(),key);
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string decrypt = entry.path().string().substr(0,entry.path().string().find_last_of('.'));
            xorDecryptFile(decrypt, key);
        }
    }
}

std::string formatFileTime(const fs::file_time_type& ftime) {
    // ?? file_time_type ???? system_clock::time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    // ????????? time_t ????????C++??
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);

    // ??? std::put_time ???��????
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void realTimeSaveFile(const fs::path& FilePath, const fs::path& savePath){
    fs::file_time_type last_write_time = fs::last_write_time(FilePath);
    fs::file_time_type pre_last_write_time = fs::last_write_time(FilePath);
    if(fs::exists(FilePath)){
        while(true){
            Sleep(5000);
            pre_last_write_time = last_write_time;
            last_write_time = fs::last_write_time(FilePath);
            if(last_write_time!=pre_last_write_time){
                std::cout<<"File changed"<<std::endl;
                copyDirectoryData(FilePath,savePath,true);
            }
        }
    }else{
        std::cout<<"File not exists"<<std::endl;
    }
    
}

void startRealTimeSave(const fs::path& FilePath, const fs::path& savePath){
    std::thread a(realTimeSaveFile,FilePath,savePath);
    a.join();
}


void startServer(){
    cout<<"server started"<<endl;
    std::thread b(server);
    b.join();
}

void startClientSending(fs::path& sourceFilePath,std::string& targetDirPath){
    std::thread c(clientSendtoServer,std::ref(sourceFilePath),std::ref(targetDirPath));
    c.join();
}


//????????????????
void realTimeSaveFileOnCloud(fs::path& FilePath, std::string& targetDirPath){
    std::cout<<"realTimeSaveFileOnCloud is On:"<<std::endl;
    std::cout<<"SourceFilePath:"<<FilePath<<std::endl;
    std::cout<<"targetDirPathOnCloud:"<<targetDirPath<<std::endl;
    fs::file_time_type last_write_time = fs::last_write_time(FilePath);
    fs::file_time_type pre_last_write_time = fs::last_write_time(FilePath);
    if(fs::exists(FilePath)){
        while(true){
            Sleep(5000);
            pre_last_write_time = last_write_time;
            last_write_time = fs::last_write_time(FilePath);
            if(last_write_time!=pre_last_write_time){
                std::cout<<"File changed"<<std::endl;
                std::thread d(clientSendtoServer,std::ref(FilePath),std::ref(targetDirPath));
                d.join();
            }
        }
    }else{
        std::cout<<"File not exists"<<std::endl;
    }
    
}

void startRealTimeSaveOnCloud(fs::path& FilePath, std::string& targetDirPath){
    std::thread e(realTimeSaveFileOnCloud,std::ref(FilePath),std::ref(targetDirPath));
    e.join();
}

void help()
{
    cout<<"-------------------------"<<endl;
    cout<<"-h  --help: show help"<<endl;
    cout<<"-v  --version: show version"<<endl;
    cout<<"-b  --backup: backup file or directory"<<endl<<"\tUsage: -b [Path] [targetDirPath -d:default]"<<endl;
    cout<<"-bf --backupFile: choose file to backup "<<endl<<"\tUsage: -bf [filePath] [targetDirPath -d:default]"<<endl;
    cout<<"-bd --backupDirectory: choose directory to backup"<<endl<<"\tUsage: -bd [dirPath] [targetDirPath -d:default]"<<endl;
    cout<<"-r  --restore: restore file"<<endl<<"\tUsage: -r then choose the record you need"<<endl;
    cout<<"-s  --server: start server"<<endl;
    cout<<"-bc --backupOnCloud: send file to server"<<endl<<"\tUsage: -bc [filePath] [targetDirPathOnHost -d:default]"<<endl;
    cout<<"-rc  --restoreOnCloud: restore file from server"<<endl<<"\tUsage: -rc then choose the record you need"<<endl;
    cout<<"-e  --encrypt: encrypt file"<<endl<<"\tUsage: -e [filePath] [key]"<<endl;
    cout<<"-d  --decrypt: decrypt file"<<endl<<"\tUsage: -d [filePath with out .encrypted] [key]"<<endl;
    cout<<"-l  --log: show log"<<endl;
    cout<<"-t  --time: real time save"<<endl<<"\tUsage: -t [filePath You Need To Save In Realtime] [targetDirPath -d:default]"<<endl;
    cout<<"-tc --timeCloud: real time save on cloud"<<endl<<"\tUsage: -tc [filePath You Need To Save In Realtime] [targetDirPathOnHost]"<<endl;
    cout<<"-sd --showDefault: show default path"<<endl;
    cout<<"-------------------------"<<endl;
}


int main(int argc, char *argv[]) {

    

    std::ifstream fileStreamPath(getExePath()+"/config", std::ios::binary);
    std::string defaultPath_str;
    std::getline(fileStreamPath,defaultPath_str);
    std::getline(fileStreamPath,defaultPath_str);
    fs::path defaultPath = defaultPath_str;

    std::ifstream fileStreamIP(getExePath()+"/config", std::ios::binary);
    std::string serverIP_str;
    std::getline(fileStreamIP,serverIP_str);

    std::ifstream fileStreamHostPath(getExePath()+"/config", std::ios::binary);
    std::string defaultHostPath_str;
    std::getline(fileStreamHostPath,defaultHostPath_str);
    std::getline(fileStreamHostPath,defaultHostPath_str);
    std::getline(fileStreamHostPath,defaultHostPath_str);
    char hideConsole_flag='i';


   
   if(argc==1){
       help();
       std::cout<<"`backup` command is add to your computer, you can use it in cmd or powershell"<<std::endl;
       system("pause");
       return 0;
   }

    // ???????��??????��???
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            help();
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "backup --vision1.0\n";
        } else if (arg == "-b" || arg == "--backup") {
            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"more path expected"<<std::endl;
                return 0;
            }
            std::string targetDirPath = argv[argc-1];
            fs::path sourceFilePath = fs::path(filePath);
            if(targetDirPath=="-d"){
                targetDirPath=defaultPath_str.substr(0,defaultPath_str.size()-1);
            }
            fs::path targetDirPath_ = fs::path(targetDirPath);
            copyDirectoryData(sourceFilePath,targetDirPath_,true);
            return 0;
        } else if (arg == "-bf" || arg == "--backupFile") {
            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"more path expected"<<std::endl;
                return 0;
            }
            std::string targetDirPath = argv[argc-1];
            fs::path sourceFilePath = fs::path(filePath);
            if(targetDirPath=="-d"){
                targetDirPath=defaultPath_str.substr(0,defaultPath_str.size()-1);
            }
            fs::path targetDirPath_ = fs::path(targetDirPath);
            copyDirectoryData(sourceFilePath,targetDirPath_,true);
            return 0;
        } else if (arg == "-bd" || arg == "--backupDirectory") {
            std::string dirPath = argv[argc-2];
            if(dirPath[0]=='-'||(dirPath[0]=='b'&&dirPath[1]=='a')){
                std::cout<<"more path expected"<<std::endl;
                return 0;
            }
            std::string targetDirPath = argv[argc-1];
            fs::path sourceDirPath = fs::path(dirPath);
            if(targetDirPath=="-d"){
                targetDirPath=defaultPath_str.substr(0,defaultPath_str.size()-1);
            }
            fs::path targetDirPath_ = fs::path(targetDirPath);
            copyDirectoryData(sourceDirPath,targetDirPath_,true);
            return 0;
        } else if (arg == "-s" || arg == "--server") {
            startServer();
            return 0;
        } else if (arg == "-bc" || arg == "--backupOnCloud") {
            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"more path expected"<<std::endl;
                return 0;
            }
            std::string targetDirPath = argv[argc-1];
            fs::path sourceFilePath = fs::path(filePath);
            if(targetDirPath=="-d"){
                targetDirPath=defaultHostPath_str.substr(0,defaultHostPath_str.size()-1);
            }
            startClientSending(sourceFilePath,targetDirPath);
            return 0;
        } else if (arg == "-r" || arg == "--restore") {
            restoreFile();
            return 0;
        } else if (arg == "-rc" || arg == "--restoreOnCloud") {
            catchCloudFileByLog();
            return 0;
        } else if (arg == "-e" || arg == "--encrypt") {
            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"path or key expected"<<std::endl;
                return 0;
            }
            std::string key = argv[argc-1];
            fs::path sourceFilePath = fs::path(filePath);
            encryptDirectoryData(sourceFilePath,key);
            return 0;
        } else if (arg == "-d" || arg == "--decrypt") {
            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"path or key expected"<<std::endl;
                return 0;
            }
            std::string key = argv[argc-1];
            fs::path sourceFilePath = fs::path(filePath);
            decryptDirectoryData(sourceFilePath,key);
            return 0;
        } else if (arg == "-l" || arg == "--log") {
            showLog();
            return 0;
        } else if (arg == "-t" || arg == "--time") {

            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"more path expected"<<std::endl;
                return 0;
            }
            std::string targetDirPath = argv[argc-1];
            fs::path sourceFilePath = fs::path(filePath);
            if(targetDirPath=="-d"){
                targetDirPath=defaultPath_str;
            }

            std::cout << "-hide the console?(y/n)-" << std::endl;
            if(hideConsole_flag=='i')
            std::cin>>hideConsole_flag;
            if (hideConsole_flag=='y')
            {
                HWND hWnd = GetConsoleWindow();
            ShowWindow(hWnd, SW_HIDE);
            } 
            else
            {
                std::cout<<"console is not hidden"<<std::endl;
            }

            fs::path targetDirPath_ = fs::path(targetDirPath);
            startRealTimeSave(sourceFilePath,targetDirPath_);

            //return 0;
        } else if(arg == "-tc" || arg == "--timeCloud"){

            std::string filePath = argv[argc-2];
            if(filePath[0]=='-'||(filePath[0]=='b'&&filePath[1]=='a')){
                std::cout<<"more path expected"<<std::endl;
                return 0;
            }
            std::string targetDirPath = argv[argc-1];
            if(targetDirPath=="-d"){
                targetDirPath=defaultHostPath_str;
                cout<<"defaultTarPathOnCloud:"<<defaultHostPath_str<<endl;
            }

            std::cout << "-hide the console?(y/n)-" << std::endl;
            if(hideConsole_flag=='i')
            std::cin>>hideConsole_flag;
            if (hideConsole_flag=='y')
            {
                HWND hWnd = GetConsoleWindow();
            ShowWindow(hWnd, SW_HIDE);
            } 
            else
            {
                std::cout<<"console is not hidden"<<std::endl;
            }

            fs::path sourceFilePath = fs::path(filePath);
            startRealTimeSaveOnCloud(sourceFilePath,targetDirPath);
            
            //return 0;
        }  else if(arg == "-sd" || arg == "--showDefault"){
            std::cout<<"default IP: "<<serverIP_str<<std::endl;
            std::cout<<"default path: "<<defaultPath.string()<<std::endl;
            std::cout<<"default host path: "<<defaultHostPath_str<<std::endl;
        }
        else {
            std::cerr << "unknown :" << arg << "\n";
            return 1;
        }
    }
    return 0;
}