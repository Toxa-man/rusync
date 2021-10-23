#pragma once
#include <string>
#include <filesystem>


namespace rusync {
namespace fs = std::filesystem;

struct Config {
    std::string ip;
    std::string port;
    fs::path path;
};


inline Config parse_config(char** argv) {
    return {
        argv[1],
        argv[2],
        argv[3]
    };
}
}

