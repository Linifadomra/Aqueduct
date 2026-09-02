#pragma once
#include <string>

namespace aqueduct {

bool configureFromFile(const std::string& confPath);
bool save(const std::string& localPath, const std::string& remoteName, const std::string& remotePath);
bool fetch(const std::string& remoteName, const std::string& remotePath, const std::string& localPath);

}