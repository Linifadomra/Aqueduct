#include "aqueduct/aqueduct.hpp"
#include "internal/librclone_shim.h"

#include <stdio.h>
#include <filesystem>

namespace {

std::string jsonEscape(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char c : in) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            default:   out += c;
        }
    }
    return out;
}

bool splitLocalPath(const std::string& path, std::string& dir, std::string& name) {
    std::filesystem::path p(path);
    if (!p.has_filename()) return false;
    dir = p.parent_path().empty() ? "." : p.parent_path().string();
    name = p.filename().string();
    return true;
}

bool copyFile(const std::string& src_fs, const std::string& src_remote,
              const std::string& dst_fs, const std::string& dst_remote) {
    std::ostringstream input;
    input << "{"
          << "\"srcFs\":\""     << jsonEscape(src_fs)     << "\","
          << "\"srcRemote\":\"" << jsonEscape(src_remote) << "\","
          << "\"dstFs\":\""     << jsonEscape(dst_fs)     << "\","
          << "\"dstRemote\":\"" << jsonEscape(dst_remote) << "\""
          << "}";
 
    std::string method = "operations/copyfile";
    std::string body   = input.str();
 
    RcloneRPCResult result = RcloneRPC(const_cast<char*>(method.c_str()),
                                        const_cast<char*>(body.c_str()));
    bool ok = (result.Status == 200);
    if (!ok && result.Output) {
        fprintf(stderr, "aqueduct: copyfile failed: %s\n", result.Output);
    }
    if (result.Output) {
        RcloneFreeString(result.Output);
    }
    return ok;
}

bool g_configured = false;

} // namespace

namespace acqueduct {

bool configureFromFile(const std::string &confPath) {
    if (g_configured) return 0; /* idempotent */

    if (setenv("RCLONE_CONFIG", confPath.c_str(), /*overwrite=*/1) != 0) {
        return false;
    }
 
    if (librclone_ensure_loaded() != 0) {
        return false;
    }
 
    RcloneInitialize();
    g_configured = true;
    return true;
}

bool save(const std::string& localPath, const std::string& remoteName, const std::string& remotePath) {
   if (!g_configured) return false;

    std::string dir, name;
    if (!splitLocalPath(localPath, dir, name)) return -1;
 
    std::string dstFs = remoteName + ":";
    std::string dstDir = remotePath;
    if (!dstDir.empty() && dstDir.back() != '/') dstDir += '/';
 
    return copyFile(dir, name, dstFs + dstDir, name) ? 0 : -1;
}

bool fetch(const std::string& remoteName, const std::string& remotePath, const std::string& localPath) {
    if (!g_configured) return -1;
 
    std::string dir, name;
    if (!splitLocalPath(localPath, dir, name)) return -1;
 
    // remotePath here is the full remote file path (dir + filename),
    // since fetch needs to know exactly which file to pull.
    std::filesystem::path rp(remotePath);
    std::string srcDir  = rp.parent_path().empty() ? "" : rp.parent_path().string() + "/";
    std::string srcName = rp.filename().string();
    std::string srcFs   = std::string(remoteName) + ":" + srcDir;
 
    return copyFile(srcFs, srcName, dir, name) ? 0 : -1;
}

}
