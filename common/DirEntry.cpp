#include "DirEntry.hpp"
#include "Utils.hpp"

namespace rusync {

DirEntry::DirEntry(const std::string& path,
        DirEntry::Type type,
        uint64_t hash
) : path {path}, type {type}, hash {hash} {

}

std::string DirEntry::type_str() const {
    if (type == FILE) {
        return "file";
    }
    return "dir";
}

DirEntry DirEntry::from_path(fs::path path, fs::path origin) {
    XXH64_hash_t hash = 0;
    if (fs::is_regular_file(path)) {
        std::ifstream file {path};
        std::vector<char> buffer;
        buffer.resize(fs::file_size(path));
        file.read(buffer.data(), buffer.size());
        hash = XXH64(buffer.data(), buffer.size(), 0);
    }
    
    return {truncate_path(path, origin), fs::is_directory(path) ? DirEntry::DIR : DirEntry::FILE, hash};
}
}