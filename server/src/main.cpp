#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "nghttp2/asio_http2.h"
#include <bits/stdint-uintn.h>
#include <nghttp2/asio_http2_server.h>
#include <filesystem>
#include <iterator>
#include <set>
#include <xxhash.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <string>
#include "DirEntry.hpp"
#include "FileChunk.hpp"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "BinaryWriter.hpp"
#include "Config.hpp"
#include "ServerSync.hpp"
#include <syncstream>


namespace rusync {

namespace fs = std::filesystem;
namespace asio = boost::asio;
namespace sys = boost::system;







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