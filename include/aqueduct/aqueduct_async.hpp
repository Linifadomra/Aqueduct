#include "aqueduct/aqueduct.hpp"
#include <future>

namespace aqueduct {

inline std::future<bool> saveAsync(const std::string& localPath, const std::string& remoteName, const std::string& remotePath) {
    return std::async(std::launch::async, save, localPath, remoteName, remotePath);
}

inline std::future<bool> fetchAsync(const std::string& remoteName, const std::string& remotePath, const std::string& localPath) {
    return std::async(std::launch::async, fetch, remoteName, remotePath, localPath);
}

}
