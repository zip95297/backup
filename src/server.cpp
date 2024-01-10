#include <iostream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include<stdlib.h>
#include<thread>
#include<vector>
#include<direct.h>


#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

namespace fs = std::filesystem;


void server() {
    WSADATA wsaData;
    SOCKET listeningSocket = INVALID_SOCKET, newSocket = INVALID_SOCKET;
    struct sockaddr_in server;

    // 初始化Winsock
    WSAStartup(MAKEWORD(2,2), &wsaData);

    // 创建socket
    listeningSocket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    // 绑定socket
    bind(listeningSocket, (struct sockaddr *)&server, sizeof(server));
    listen(listeningSocket, 3);

    char hideConsole_flag='i';
    while(true)
    {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Listening... Waiting for incoming connections..." << std::endl;
    std::cout << "-hide the console?(y/n)-" << std::endl;
    // 接受连接
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
    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;
    newSocket = accept(listeningSocket, (struct sockaddr *)&client, &c);
    std::cout << "TCP connection is contributed!" << std::endl;
    std::cout << "waiting for target file path..." << std::endl;
    uint32_t filePathLength;
    recv(newSocket, reinterpret_cast<char*>(&filePathLength), sizeof(filePathLength), 0);
    std::string filePath(filePathLength, '\0');
    recv(newSocket, &filePath[0], filePathLength, 0);
    std::cout << "target file path is " << filePath << std::endl;


    if(fs::path(filePath).filename()=="getFileRequest"){
        std::cout<<"getFileRequest received"<<std::endl;
        std::cout<<"sending file..."<<std::endl;
        std::string sourceFilePathOnHost=fs::path(filePath).parent_path().string();
        //发送sourceFilePathOnHost

    // 打开文件并读取内容
    std::ifstream fileStream(sourceFilePathOnHost, std::ios::binary);
    if (fileStream.is_open()) {
        std::cout << "File opened successfully.sending..." << std::endl;
        std::string fileContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        // 发送文件内容



        int res=send(newSocket, fileContent.c_str(), fileContent.size(), 0);
        std::cout<<"send result:"<<res<<std::endl;

    } else {
        std::cerr << "Cannot open file: " << filePath << std::endl;
    }

    Sleep(25);

    std::cout << "File sent successfully." << std::endl;
        continue;
    }
        
    //filePath是文件位置=目标目录位置/文件名
    //下面判断目录是否存在，不存在则创建
    fs::path targetDirPath = fs::path(filePath).parent_path();
    if(!fs::exists(targetDirPath)){
        std::cout<<"target directory does not exist, creating..."<<std::endl;
        fs::create_directories(targetDirPath);
    }
    //判断文件是否存在，存在则覆盖
    if(fs::exists(filePath)){
        std::cout<<"target file already exists, overwriting..."<<std::endl;
    }
    

    std::ofstream file(filePath, std::ios::binary); // 使用接收到的路径创建文件

    char buffer[1024];
    int bytesRead;

    while ((bytesRead = recv(newSocket, buffer, 1024, 0)) > 0) {
        file.write(buffer, bytesRead);
    }


    }
    closesocket(newSocket);
    closesocket(listeningSocket);
    WSACleanup();

    return ;
}

//cd "c:\Users\zjb\Desktop\soft\backup\" ; if ($?) { g++ -std=c++17 server.cpp -o server -lwsock32 -lws2_32} ;if ($?) { .\server }