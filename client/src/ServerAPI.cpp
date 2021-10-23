#include "ServerAPI.hpp"
#include <string>
#include <syncstream>
#include "BinaryParser.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/post.hpp"
#include "nghttp2/asio_http2.h"

namespace rusync {
ServerAPI::ServerAPI(const Config& conf, boost::asio::io_service& io_service) : 
    m_conf(conf), m_retry_timer(io_service, CONNECTION_RETRY_TIMEOUT) {
    create_session(io_service);
}

ServerAPI::~ServerAPI() {
    m_session->shutdown();
}

void ServerAPI::create_session(boost::asio::io_service &io_service ) {
    m_session = std::make_unique<nghttp2::asio_http2::client::session>(io_service, m_conf.server_host, m_conf.server_port);
    m_session->on_connect([this](boost::asio::ip::tcp::resolver::iterator endpoint_it) {
        std::osyncstream(std::cout) << "Successfully connected to " <<  m_conf.server_host << ":" << m_conf.server_port << std::endl;
        m_connected = true;
    });
    m_session->on_error([this, &io_service](const boost::system::error_code &ec) {
        std::osyncstream(std::cout) << "Error occured on connecting to " << m_conf.server_host << ":" << m_conf.server_port << " : " << ec.message() << 
            ". Will retry in " << CONNECTION_RETRY_TIMEOUT.total_seconds() << " seconds" << std::endl;
        retry_connection(io_service);
    });
}

void ServerAPI::retry_connection(boost::asio::io_service &io_service) {
    m_retry_timer.expires_at(m_retry_timer.expires_at() + CONNECTION_RETRY_TIMEOUT);
    m_retry_timer.async_wait([this, &io_service](const boost::system::error_code & err) {
        if (err == boost::asio::error::operation_aborted) {
            return;
        }
        create_session(io_service);
    });
}

void ServerAPI::get_files_description(GetFilesDescriptionCallback cb) {
    perform_http_request(DESCRIPTION_PATH, "GET", [cb](std::vector<char> data){
        std::osyncstream(std::cout) << "Recevied file descriptors, data: " << std::string(data.data(), data.size()) << std::endl;
        rapidjson::Document document;
        document.Parse(data.data(), data.size());
        if (!document.IsArray()) {
            std::cerr << "Invalid JSON schema in response" << std::endl;
            return;
        }
        std::set<DirEntry> remote_entries;
        for (const auto& entry: document.GetArray()) {
            remote_entries.insert({
                entry["path"].GetString(),
                entry["type"] == "file" ? DirEntry::FILE : DirEntry::DIR,
                entry["hash"].GetUint64()
            });
        }
        cb(std::move(remote_entries));
    });
    
}
bool ServerAPI::connected() const {
    return m_connected;
}

void ServerAPI::upload_file(std::string file, const std::string& path) {
    QueryParamsMap params;
    params["path"] = path;
    params["type"] = "file";
    perform_http_request(FILES_PATH, "POST", std::move(params), std::move(file));
}

void ServerAPI::remove_file(const std::string& path) {
    QueryParamsMap params;
    params["path"] = path;
    perform_http_request(FILES_PATH, "DELETE", std::move(params));
}

void ServerAPI::upload_dir(const std::string& path) {
    QueryParamsMap params;
    params["path"] = path;
    params["type"] = "dir";
    perform_http_request(FILES_PATH, "POST", std::move(params));
}

void ServerAPI::get_file(const std::string& path, GetFileCallback cb) {
    QueryParamsMap params;
    params["path"] = path;
    perform_http_request(FILES_PATH, "GET", std::move(params), "", [cb, path](std::vector<char> data){
        std::osyncstream(std::cout) << "Recevied file with path: " << path << ", size: " << data.size() << std::endl;
        cb(std::move(data));
    });
}

void ServerAPI::upload_patch(const std::string& path, uint64_t offset, std::string data, bool end) {
    QueryParamsMap params;
    params["path"] = path;
    params["offset"] = std::to_string(offset);
    if (end) {
        params["end"] = "1";
    }
    perform_http_request(FILES_PATH, "PATCH", std::move(params), std::move(data));
}

void ServerAPI::get_meta(const std::string& path, GetMetaCallback cb) {
    QueryParamsMap params;
    params["path"] = path;
    perform_http_request(META_PATH, "GET", std::move(params), "", [cb, path](std::vector<char> data){
        std::osyncstream(std::cout) << "Recevied meta object with path: " << path << ", size: " << data.size() << std::endl;
        std::vector<FileChunk> remote_chunks;
        BinaryParser parser {reinterpret_cast<const unsigned char*>(data.data()), data.size()};
        const bool is_file = parser.read<uint8_t>();
        while(parser.get_bytes_remain() != 0) {
            remote_chunks.push_back({parser.read<uint32_t>(), parser.read<uint64_t>()});
        }
        cb(is_file, std::move(remote_chunks));
    });
}



void ServerAPI::perform_http_request(const std::string& path, 
                            const std::string& method,
                            ReceiveCb receive_cb) {
    return perform_http_request(path, method, QueryParamsMap(), "", std::move(receive_cb));
}

void ServerAPI::perform_http_request(const std::string& path, 
                            const std::string& method, 
                            QueryParamsMap&& query,
                            std::string data,
                            ReceiveCb receive_cb) {
    const std::string query_params = std::accumulate(query.begin(), query.end(), "key=" + m_conf.key, [](const std::string accum, const auto& pair) {
        return accum + "&" + pair.first + "=" + pair.second;
    });
    std::string uri = "http://" + m_conf.server_host + ":" + m_conf.server_port + path + "?" + url_encode(query_params);
    boost::system::error_code ec;
    const auto* req = m_session->submit(ec, method, uri, std::move(data));
    if (!req) {
        std::osyncstream(std::cerr) << "Failed to perform request for " << uri << ", reason: " << ec.message() << std::endl;
        return;
    }
    std::osyncstream(std::cout) << "Performing " << method << " request to " << uri << std::endl;
    req->on_response([receive_cb](const nghttp2::asio_http2::client::response& resp){
        std::osyncstream(std::cout) << "Got status_code " << resp.status_code() << std::endl;
        if (resp.status_code() != 200) {
            return;
        }
        if (receive_cb) {
            on_full_data(receive_cb, resp);
        }
    });
    return;

}

}