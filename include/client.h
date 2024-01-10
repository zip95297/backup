#include<iostream>
#include<fstream>
#include<filesystem>
#include<sstream>
namespace fs = std::filesystem;
#ifndef CLIENT_H
#define CLIENT_H
std::string getExePath();
std::string timeNow();
void writeCloudLog(const std::string& content);
void clientSendtoServer(fs::path& sourceFilePath,std::string& targetDirPath);
void clientCatchServer(fs::path& sourceFilePathOnHost,std::string& targetDirPath);
void catchCloudFileByLog();
#endif // CLIENT_H