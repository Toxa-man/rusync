#include "ServerSync.hpp"
#include <filesystem>

namespace rusync {
ServerSync::ServerSync(const Config& config) : m_conf {config} {
    m_server.num_threads(std::thread::hardware_concurrency());
    m_server.handle(FILES_PATH, [this](const auto&... args) {
        handle_files_request(args...);
    });
    m_server.handle(DESCRIPTION_PATH, [this](const auto&... args) {
        handle_files_description_request(args...);
    });
    m_server.handle(META_PATH, [this](const auto&... args) {
        handle_meta_request(args...);
    });
}

void ServerSync::listen() {
    boost::system::error_code ec;
    std::osyncstream(std::cout) << "Server is listening" << std::endl;
    if (m_server.listen_and_serve(ec, m_conf.ip, m_conf.port)) {
        std::cerr << "error: " << ec.message() << std::endl;
    }
}

void ServerSync::handle_file_upload(const nghttp2::asio_http2::server::request &req, 
                        const nghttp2::asio_http2::server::response &res,
                        const ServerSync::QueryParams& query_params,
                        const fs::path& full_path) {
    if (query_params.at("type") == "dir") {
        std::filesystem::create_directories(full_path.parent_path());
        fs::create_directory(full_path);
        std::osyncstream(std::cout) << "Created dir at path: " << full_path << std::endl;
        return;
    }
    on_full_data([full_path, &res](std::vector<char> buffer) {
        std::filesystem::create_directories(full_path.parent_path());
        std::fstream stream {full_path, std::ios::binary | std::ios::out };
        stream.write(buffer.data(), buffer.size());
        std::osyncstream(std::cout) << "Created file " << full_path << " size: " << buffer.size() << " bytes" << std::endl;
        res.write_head(200);
        res.end();
    }, req);
}

void ServerSync::handle_file_patch(const nghttp2::asio_http2::server::request &req, 
                        const nghttp2::asio_http2::server::response &res,
                        const ServerSync::QueryParams& query_params,
                        const fs::path& full_path) {
    on_full_data([&res, query_params, this, full_path](std::vector<char> buffer) {
        if (fs::is_directory(full_path)) {
            res.write_head(200);
            res.end();
            return;
        }
        const uint64_t offset = std::stoull(query_params.at("offset"));
        std::fstream stream {full_path, std::ios::binary | std::ios::out | std::ios::in};
        stream.seekp(offset);
        stream.write(buffer.data(), buffer.size());
        std::osyncstream(std::cout) << "Patching file " << full_path << " with " << buffer.size() << " bytes at offset: " << offset << std::endl; 
        if (query_params.contains("end") && query_params.at("end") == "1" 
            && stream.tellp() != fs::file_size(full_path) // here we cutting the end of file
            ) {
            const auto old_size = fs::file_size(full_path);
            std::vector<char> temp_file_buffer;
            temp_file_buffer.resize(stream.tellp());
            stream.seekg(0);
            stream.read(temp_file_buffer.data(), temp_file_buffer.size());
            stream.close();
            stream.open(full_path, std::ios::binary | std::ios::out);
            stream.write(temp_file_buffer.data(), temp_file_buffer.size());
            std::osyncstream(std::cout) << "Resizing " << full_path << " from " << old_size << " to " << temp_file_buffer.size() << std::endl;
        }
        res.write_head(200);
        res.end();
    }, req);
}

void ServerSync::handle_file_download(const nghttp2::asio_http2::server::request &req, 
                        const nghttp2::asio_http2::server::response &res,
                        const ServerSync::QueryParams& query_params,
                        const fs::path& full_path) {
    if (!fs::exists(full_path)) {
        res.write_head(404);
        res.end();
        return;
    }
    std::ifstream stream {full_path, std::ios::binary};
    std::string buffer;
    buffer.resize(fs::file_size(full_path));
    stream.read(buffer.data(), buffer.size());
    res.write_head(200);
    res.end(std::move(buffer));
}

void ServerSync::handle_file_removal(const nghttp2::asio_http2::server::request &req, 
                        const nghttp2::asio_http2::server::response &res,
                        const ServerSync::QueryParams& query_params,
                        const fs::path& full_path) {
    if (!fs::exists(full_path)) {
        res.write_head(404);
        res.end();
        return;
    }
    std::osyncstream(std::cout) << "Removing " << (fs::is_regular_file(full_path) ? " file " : " dir ") << full_path << std::endl;
    fs::remove_all(full_path);
    res.write_head(200);
    res.end();        
} 

void ServerSync::handle_files_request(const nghttp2::asio_http2::server::request &req, const nghttp2::asio_http2::server::response &res) {
    std::osyncstream(std::cout) << "Request to " << req.method() << " files api, uri: " << uri_obj_to_str(req.uri()) << std::endl;
    auto query_params = parse_params(nghttp2::asio_http2::percent_decode(req.uri().raw_query));
    fs::path full_path = m_conf.path / fs::path(query_params.at("key")) / query_params.at("path");
    if (req.method() == "POST") {
        handle_file_upload(req, res, query_params, full_path);
    } else if (req.method() == "PATCH") {
        handle_file_patch(req, res, query_params, full_path);
    } else if (req.method() == "GET") {
        handle_file_download(req, res, query_params, full_path);
    } else if (req.method() == "DELETE") {
        handle_file_removal(req, res, query_params, full_path);
    } else {
        res.write_head(405);
        res.end();
    }
}
void ServerSync::handle_files_description_request(const nghttp2::asio_http2::server::request &req, const nghttp2::asio_http2::server::response &res) {
    std::osyncstream(std::cout) << "Request to " << req.method() << " files description, uri: " << uri_obj_to_str(req.uri()) << std::endl;
    auto query_params = parse_params(nghttp2::asio_http2::percent_decode(req.uri().raw_query));
    if (req.method() != "GET") {
        res.write_head(405);
        res.end();
        return;
    }
    rapidjson::Document doc(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    if (fs::exists(m_conf.path / query_params["key"])) {
        auto local_entries = extract_entries_from_path<std::vector<DirEntry>>(m_conf.path / query_params["key"]);
        for (const auto& entry: local_entries) {
            rapidjson::Value obj;
            obj.SetObject();
            obj.GetObject().AddMember("path", rapidjson::Value().SetString(entry.path.c_str(), alloc), alloc);
            obj.GetObject().AddMember("type", rapidjson::Value().SetString(entry.type_str().c_str(), alloc), alloc);
            obj.GetObject().AddMember("hash", rapidjson::Value().SetUint64(entry.hash), alloc);
            doc.PushBack(obj, alloc);
        }
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer {buffer};
    doc.Accept(writer);
    std::string res_buffer(buffer.GetString(), buffer.GetSize());
    res.write_head(200);
    res.end(std::move(res_buffer));
}

void ServerSync::handle_meta_request(const nghttp2::asio_http2::server::request &req, const nghttp2::asio_http2::server::response &res) {
    std::osyncstream(std::cout) << "Request to meta api, uri: " << uri_obj_to_str(req.uri()) << std::endl;
    auto query_params = parse_params(nghttp2::asio_http2::percent_decode(req.uri().raw_query));
    fs::path full_path = m_conf.path / fs::path(query_params.at("key")) / query_params.at("path");
    if (req.method() != "GET") {
        res.write_head(405);
        res.end();
        return;
    }
    if (!fs::exists(full_path)) {
        res.write_head(404);
        res.end();
        return;
    }
    if (!fs::is_regular_file(full_path)) {
        std::string buffer;
        buffer.resize(1);
        BinaryWriter writer {reinterpret_cast<unsigned char*>(buffer.data()), buffer.size()};
        writer.write(uint8_t{0});
        res.write_head(200);
        res.end(std::move(buffer));
        return;
    }
    std::vector<FileChunk> chunks = compute_chunks_for_file(full_path);
    std::osyncstream(std::cout) << "Computed " << chunks.size() << " chunks for " << full_path << std::endl;
    std::string result_buffer;
    result_buffer.resize(chunks.size() * (sizeof(chunks[0].size) + sizeof(chunks[0].hash)) + 1);
    BinaryWriter writer {reinterpret_cast<unsigned char*>(result_buffer.data()), result_buffer.size()};
    writer.write(uint8_t{1});
    for (const auto& chunk: chunks) {
        writer.write(chunk.size);
        writer.write(chunk.hash);
    }
    res.write_head(200);
    res.end(std::move(result_buffer));
}

size_t ServerSync::calculate_chunk_size(size_t file_size) {
    if (file_size < 1'000'000) {
        return 1000;
    } else if (file_size < 1'000'000'000) {
        return 31622;
    }
    return 100000; 
}

std::vector<FileChunk> ServerSync::compute_chunks_for_file(const fs::path& path) {
    const int chunk_size = calculate_chunk_size(fs::file_size(path));
    std::vector<FileChunk> chunks;
    FILE* stream = fopen(path.c_str(), "rb");
    std::vector<char> buffer;
    buffer.resize(chunk_size);
    while(!feof(stream)) {
        const auto bytes_read = fread(buffer.data(), 1, buffer.size(), stream);
        if (bytes_read > 0) {
            const auto hash = XXH64(buffer.data(), bytes_read, 0);
            chunks.push_back({static_cast<uint32_t>(bytes_read), hash});
        }
    }
    fclose(stream);
    return chunks;
}


std::string ServerSync::uri_obj_to_str(const nghttp2::asio_http2::uri_ref& uri) {
    return uri.scheme + "://" + uri.host + uri.path + "?" + nghttp2::asio_http2::percent_decode(uri.raw_query);
}

ServerSync::QueryParams ServerSync::parse_params(std::string_view query) {
    QueryParams result;
    std::vector<std::string> params;
    boost::algorithm::split(params, query, boost::is_any_of("&"));
    for (const auto& param: params) {
        std::vector<std::string> key_value;
        boost::algorithm::split(key_value, param, boost::is_any_of("="));
        result[key_value[0]] = key_value[1];
    }
    return result;
}
}