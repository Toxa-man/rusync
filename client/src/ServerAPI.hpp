#pragma once
#include "Config.hpp"
#include "FileChunk.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/tcp.hpp"
#include <nghttp2/asio_http2_client.h>
#include <set>
#include "DirEntry.hpp"
#include <algorithm>
#include <functional>
#include <vector>
#include <numeric>
#include "Utils.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include <rapidjson/document.h>

namespace rusync {
/**
 * @brief Contains methods for work with remote server
 * 
 */
class ServerAPI {
public:
    ServerAPI(const Config& conf, boost::asio::io_service& io_service);
    ~ServerAPI();

    /**
     * @brief Establishes tcp connection to server
     * 
     * @param io_service 
     */
    void create_session(boost::asio::io_service& io_service);
    /**
     * @brief schedule connection retry within CONNECTION_RETRY_TIMEOUT seconds
     * 
     * @param io_service 
     */
    void retry_connection(boost::asio::io_service& io_service);

    using GetFilesDescriptionCallback = std::function<void(std::set<DirEntry>)>;
    /**
     * @brief Get the files description object as set of DirEntry and passes it into provided cb
     * 
     * @param cb 
     */
    void get_files_description(GetFilesDescriptionCallback cb);

    /**
     * @brief upload file to server
     * 
     * @param file file's content
     * @param path file's path
     */
    void upload_file(std::string file, const std::string& path);

    /**
     * @brief upload dir to server
     * 
     * @param path path to dir
     */
    void upload_dir(const std::string& path);
    using GetFileCallback = std::function<void(std::vector<char>)>;
    /**
     * @brief Downloads remote file and passes it as vector<char> to provided cb
     * 
     * @param path path to file at server
     * @param cb 
     */
    void get_file(const std::string& path, GetFileCallback cb);

    /**
     * @brief uploads file patch to server
     * 
     * @param path - path to file on server
     * @param offset - positon in bytes from which patch starts
     * @param data  - binary data to patch
     * @param end - if true - this is the last chunk, meaning that all data after that chunk will be truncated
     */
    void upload_patch(const std::string& path, uint64_t offset, std::string data, bool end = false);

    /**
     * @brief remove file using path
     * 
     * @param path 
     */
    void remove_file(const std::string& path);

    using GetMetaCallback = std::function<void(bool is_file, std::vector<FileChunk>)>;

    /**
     * @brief Get the meta object from server and pass it as vector<FileChunk> to provided cb
     * 
     * @param path 
     * @param cb 
     */
    void get_meta(const std::string& path, GetMetaCallback cb);

    /**
     * @brief 
     * 
     * @return true is currently connected to server
     * @return false otherwise
     */
    bool connected() const;

private:
    using ReceiveCb = std::function<void(std::vector<char>)>;
    using QueryParamsMap = std::map<std::string, std::string>;

    /**
     * @brief convinient method for perfoming request wihout query params
     * 
     * @param path 
     * @param method 
     * @param receive_cb 
     */
    void perform_http_request(const std::string& path, 
                              const std::string& method,
                              ReceiveCb receive_cb = ReceiveCb());

    /**
     * @brief Performs actual HTTP/2 request and passes result as vector<char> to provided receive_cb
     * 
     * @param path - http path (e.g. /files)
     * @param method - http method
     * @param query - map of params in form of [key, value]
     * @param data - optional data for request
     * @param receive_cb 
     */
    void perform_http_request(const std::string& path, 
                              const std::string& method, 
                              QueryParamsMap&& query = QueryParamsMap(),
                              std::string data = "",
                              ReceiveCb receive_cb = ReceiveCb());

    
    std::unique_ptr<nghttp2::asio_http2::client::session> m_session;
    const Config& m_conf;
    const char* FILES_PATH = "/files";
    const char* DESCRIPTION_PATH = "/files_description";
    const char* META_PATH = "/meta";
    bool m_connected = false;
    const boost::posix_time::seconds CONNECTION_RETRY_TIMEOUT = boost::posix_time::seconds{2}; 
    boost::asio::deadline_timer m_retry_timer;
};
}

