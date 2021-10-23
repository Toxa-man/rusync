#pragma once 
#include <bits/stdint-uintn.h>
#include <nghttp2/asio_http2_server.h>
#include <cstddef>
#include <filesystem>
#include "Config.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "nghttp2/asio_http2.h"
#include <iostream>
#include "DirEntry.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <boost/algorithm/string.hpp>
#include "Utils.hpp"
#include "FileChunk.hpp"
#include "BinaryWriter.hpp"
#include <syncstream>


namespace rusync {

namespace fs = std::filesystem;
/**
 * @brief Main class responsible for serving file API
 * 
 */
class ServerSync {
public:
    ServerSync(const Config& config);

    using QueryParams = std::map<std::string, std::string>;
    /**
     * @brief will block and listen on specified ip and port
     * 
     */
    void listen();
private:
    /**
     * @brief Handles POST to FILES_PATH
     * 
     * @param req 
     * @param res 
     * @param query_params 
     * @param full_path 
     */
    void handle_file_upload(const nghttp2::asio_http2::server::request &req, 
                            const nghttp2::asio_http2::server::response &res,
                            const QueryParams& query_params,
                            const fs::path& full_path);

    /**
     * @brief Handles PATCH to FILES_PATH
     * 
     * @param req 
     * @param res 
     * @param query_params 
     * @param full_path 
     */
    void handle_file_patch(const nghttp2::asio_http2::server::request &req, 
                            const nghttp2::asio_http2::server::response &res,
                            const QueryParams& query_params,
                            const fs::path& full_path);

    /**
     * @brief handles GET to FILES_PATH
     * 
     * @param req 
     * @param res 
     * @param query_params 
     * @param full_path 
     */
    void handle_file_download(const nghttp2::asio_http2::server::request &req, 
                            const nghttp2::asio_http2::server::response &res,
                            const QueryParams& query_params,
                            const fs::path& full_path);
    /**
     * @brief Handles DELETE to FILES_PATH
     * 
     * @param req 
     * @param res 
     * @param query_params 
     * @param full_path 
     */
    void handle_file_removal(const nghttp2::asio_http2::server::request &req, 
                            const nghttp2::asio_http2::server::response &res,
                            const QueryParams& query_params,
                            const fs::path& full_path);

    /**
     * @brief Handles request to FILES_PATH. Currently supports only GET, POST, PATCH and DELETE
     * 
     * @param req 
     * @param res 
     */
    void handle_files_request(const nghttp2::asio_http2::server::request &req, const nghttp2::asio_http2::server::response &res);

    /**
     * @brief Handles request to DESCRIPTION_PATH
     * 
     * @param req 
     * @param res 
     */
    void handle_files_description_request(const nghttp2::asio_http2::server::request &req, const nghttp2::asio_http2::server::response &res);

    /**
     * @brief handles request to META_PATH
     * 
     * @param req 
     * @param res 
     */
    void handle_meta_request(const nghttp2::asio_http2::server::request &req, const nghttp2::asio_http2::server::response &res);

    /**
     * @brief computes patch chunk size based of file size
     * 
     * @param file_size 
     * @return size_t 
     */
    static size_t calculate_chunk_size(size_t file_size);
    std::vector<FileChunk> compute_chunks_for_file(const fs::path& path);

    static std::string uri_obj_to_str(const nghttp2::asio_http2::uri_ref& uri);

    /**
     * @brief parsing parameters from string to map of <key, value>
     * 
     * @param query 
     * @return QueryParams 
     */
    static QueryParams parse_params(std::string_view query);

    nghttp2::asio_http2::server::http2 m_server;
    Config m_conf;
    const char* FILES_PATH = "/files";
    const char* DESCRIPTION_PATH = "/files_description";
    const char* META_PATH = "/meta";
};
}