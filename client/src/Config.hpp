#pragma once

#include <filesystem>
#include <string>

namespace rusync {
namespace fs = std::filesystem;
struct Config {
    /**
     * @brief path to client dir
     * 
     */
    fs::path path;
    std::string server_host;
    std::string server_port;
    /**
     * @brief unique identifier of current client
     * 
     */
    std::string key;
    /**
     * @brief conivinient function which converts input args to Config object
     * 
     * @param argc 
     * @param argv 
     * @return Config 
     */
    static Config from_args(int argc, char** argv) {
        return {argv[1], argv[2], argv[3], argv[4]};
    }
};
}


