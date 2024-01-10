#include <iostream>
#include <iostream>
#include <fstream>
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

namespace fs = std::filesystem;

#pragma comment(lib, "Ws2_32.lib")

std::string getExePath() {
    fs::path exePath=_pgmptr;
    return exePath.parent_path().string()+"/";
}


std::string timeNow(){
    // ??????????
    auto now = std::chrono::system_clock::now();
    // ???? time_t ?????????????
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void writeCloudLog(const std::string& content) {
    std::ofstream outputFile(getExePath()+"/Cloudlog", std::ios::app);
    if (outputFile) {
        outputFile << content <<std::endl;
        std::cout << "Record written successfully." << std::endl;
    } 
}


void clientSendtoServer(fs::path& sourceFilePath,std::string& targetDirPath) {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    std::ifstream fileStreamIP(getExePath()+"/config", std::ios::binary);
    std::string serverIP_str;
    std::getline(fileStreamIP,serverIP_str);
    const char* serverIP = serverIP_str.c_str();
    const int serverPort = 8888;
    std::string filePath = sourceFilePath.string();
    std::string targetFilePath = targetDirPath+"/"+sourceFilePath.filename().string();

    // 初始化Winsock
    WSAStartup(MAKEWORD(2,2), &wsaData);

    // 创建socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    // 连接到服务器
    if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))){
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl<<"restore failed ,please check the server status..."<<std::endl;
        return;
    }
    else
        std::cout << "Connected successfully." << std::endl;

    //发送targetFilePath
    uint32_t targetFilePathLength = static_cast<uint32_t>(targetFilePath.size());
    send(clientSocket, reinterpret_cast<char*>(&targetFilePathLength), sizeof(targetFilePathLength), 0);
    send(clientSocket, targetFilePath.c_str(), targetFilePath.size(), 0);

    // 打开文件并读取内容
    std::ifstream fileStream(filePath, std::ios::binary);
    if (fileStream.is_open()) {
        std::string fileContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

        // 发送文件内容
        send(clientSocket, fileContent.c_str(), fileContent.size(), 0);
    } else {
        std::cerr << "Cannot open file: " << filePath << std::endl;
    }

    std::string record=sourceFilePath.filename().string()+" "+sourceFilePath.string()+" "+targetDirPath+" "+timeNow();
    writeCloudLog(record);


    std::cout << "File sent successfully." << std::endl;
    // 关闭socket和清理Winsock
    closesocket(clientSocket);
    WSACleanup();

    return ;
}

void clientCatchServer(fs::path& sourceFilePathOnHost,std::string& targetDirPath) {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    std::ifstream fileStreamIP(getExePath()+"/config", std::ios::binary);
    std::string serverIP_str;
    std::getline(fileStreamIP,serverIP_str);
    const char* serverIP = serverIP_str.c_str();
    const int serverPort = 8888;
    std::string filePath = sourceFilePathOnHost.string();
    std::string targetFilePath = targetDirPath+"/"+sourceFilePathOnHost.filename().string();

    // 初始化Winsock
    WSAStartup(MAKEWORD(2,2), &wsaData);

    // 创建socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    // 连接到服务器
    if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
    else
        std::cout << "Connected successfully." << std::endl;

    sourceFilePathOnHost=sourceFilePathOnHost/"getFileRequest";

    //发送sourceFilePathOnHost
    uint32_t sourceFilePathOnHostLength = static_cast<uint32_t>(sourceFilePathOnHost.string().size());
    send(clientSocket, reinterpret_cast<char*>(&sourceFilePathOnHostLength), sizeof(sourceFilePathOnHostLength), 0);
    send(clientSocket, sourceFilePathOnHost.string().c_str(), sourceFilePathOnHost.string().size(), 0);

    //filePath是文件位置=目标目录位置/文件名
    //下面判断目录是否存在，不存在则创建
    Sleep(5);
    if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))){
    fs::path targetDirPath_ = fs::path(targetDirPath);
    if(!fs::exists(targetDirPath_)){
        std::cout<<"target directory does not exist, creating..."<<std::endl;
        fs::create_directories(targetDirPath_);
    }
    fs::path targetFilePath_ = targetDirPath_/sourceFilePathOnHost.parent_path().filename();
    //判断文件是否存在，存在则覆盖
    if(fs::exists(filePath)){
        std::cout<<"target file already exists, overwriting..."<<std::endl;
    }

    std::ofstream file(targetFilePath_, std::ios::binary); // 使用接收到的路径创建文件

    char buffer[1024];
    int bytesRead;
    std::cout<<"receiving file..."<<std::endl;

    bytesRead = recv(clientSocket, buffer, 1024, 0);
    file.write(buffer, bytesRead);
    std::cout<<"writing..."<<std::endl;
    

    std::cout << "File received successfully." << std::endl;
    // 关闭socket和清理Winsock
    closesocket(clientSocket);
    WSACleanup();
    }
    return ;
}

void catchCloudFileByLog(){
    std::ifstream file(getExePath()+"/Cloudlog");
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
    std::ifstream file2(getExePath()+"/Cloudlog");
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
    fs::path saveFilePath=savePath/path.filename();
    std::string targetDirPathStr=path.parent_path().string();
    clientCatchServer(saveFilePath,targetDirPathStr);
    std::cout<<"restore successfully"<<std::endl;
}


