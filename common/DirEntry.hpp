#pragma once
#include <cassert>
#include <string>
#include <set>
#include <xxhash.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <syncstream>
#include <boost/assert.hpp>

namespace rusync {

namespace fs = std::filesystem;

/**
 * @brief described entry within file system
 * 
 */
struct DirEntry {
    enum Type {FILE, DIR} type;
    DirEntry(const std::string& path, Type type, uint64_t hash);
    
    /**
     * @brief truncated path to entry (without path to client dir)
     * 
     */
    std::string path;

    /**
     * @brief Entry hash. For files computed with XXH64 alg with seed=0. For dirs hash is always 0
     * 
     */
    uint64_t hash;
    
    /**
     * @brief converts entry type to string
     * 
     * @return std::string 
     */
    std::string type_str() const;

    /**
     * @brief Created DirEntry object from path
     * 
     * @param path - path to entry
     * @param origin - path to client dir. Need to perform truncation.<br>
     * e.g. if path == "client_dir/test.txt" and origin == "client_dir" result DirEntry.path will be equal to "test.txt"
     * @return DirEntry 
     */
    static DirEntry from_path(fs::path path, fs::path origin);
};



inline bool operator < (const DirEntry& l, const DirEntry& r) {
    return l.path < r.path;
}

inline bool operator == (const DirEntry& l, const DirEntry& r) {
    return l.hash == r.hash && l.path == r.path && l.type == r.type; 
}

/**
 * @brief Extracts entries from path and returns it as container T of DirEntry. Only counts regular files and dirs
 * 
 * @tparam T 
 * @param path 
 * @return T 
 */
template <typename T = std::set<DirEntry>> 
T extract_entries_from_path(const std::string& path) {
    T result;
    try {
        for (const auto& entry: fs::recursive_directory_iterator{path}) {
            if (!entry.is_directory() && !entry.is_regular_file()) {
                continue;
            }
            try {
                if constexpr (std::is_same_v<T, std::set<DirEntry>>) {
                    result.insert(DirEntry::from_path(entry, path));
                } else {
                    result.push_back(DirEntry::from_path(entry, path));
                }
            } catch (fs::filesystem_error& err) {
                std::osyncstream(std::cerr) << "Error occured while extracting entry from  " << entry.path() << ", " << err.what() << std::endl; 
            }
        } 
    } catch (fs::filesystem_error& err) {
        std::osyncstream(std::cerr) << "Error occured while iterating over  " << path << ", " << err.what() << std::endl; 
    }
    return result;
}

}

