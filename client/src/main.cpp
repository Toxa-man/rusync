#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include <nghttp2/asio_http2_client.h>
#include <filesystem>
#include <iterator>
#include <set>
#include <xxhash.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <string>
#include <syncstream>
#include <thread>
#include "SyncApp.hpp"
#include "Config.hpp"

namespace fs = std::filesystem;
namespace asio = boost::asio;
namespace sys = boost::system;


namespace rusync {

void sigtermHandler( int signum) {
    stopped = true;
    std::osyncstream(std::cout) << "Cought SIGTERM" << std::endl;
}


int start(int argc, char** argv) {
    signal(SIGTERM, sigtermHandler);
    if (argc != 5) {
        std::osyncstream(std::cout) << "Usage: rusync_client <path/to/folder> <server_ip> <port> <key>" << std::endl;
        return -1; 
    }
    Config conf = Config::from_args(argc, argv);
    if (!fs::exists(conf.path)) {
        std::cerr << "Please provide existing input directory" << std::endl;
        return -2;
    }
    SyncApp app {conf};
    std::osyncstream(std::cout) << "Client exiting..." << std::endl;
    
    return 0;

}


}


int main(int argc, char** argv) {
    rusync::start(argc, argv);

}