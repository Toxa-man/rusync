#include <syncstream>
#include <iostream>
#include "Config.hpp"
#include "ServerSync.hpp"

namespace rusync {


int start(int argc, char** argv) {
    if (argc != 4) {
        std::osyncstream(std::cout) << "Usage: rusync_server <ip> <port> <path/to/dir>" << std::endl;
        return -1; 
    }


    Config conf = parse_config(argv);

    ServerSync server {conf};

    server.listen();
    std::osyncstream(std::cout) << "Server Exiting..." << std::endl;

    return 0;

}


}

int main(int argc, char** argv) {
    return rusync::start(argc, argv);
}