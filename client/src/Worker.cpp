#include "Worker.hpp"


namespace rusync {

namespace fs = std::filesystem;

Worker::Worker(const Config& conf) : 
m_thread{m_io_service}, m_conf {conf} {
    boost::asio::post(m_io_service, [this]() {
        m_api = std::make_unique<ServerAPI>(m_conf, m_io_service);
    });
}

Worker::~Worker() {
    m_io_service.stop();
    std::osyncstream(std::cout) << "~Worker" << std::endl;
}


void Worker::file_added(const fs::path& path) {
    if (!fs::exists(m_conf.path / path)) {
        std::osyncstream(std::cout) << "File " << path << " was added, but now seems like it's gone" << std::endl;
        return;
    }
    if (fs::is_regular_file(m_conf.path / path)) {
        std::osyncstream(std::cout) << "File " << path << " was added, uploading it to server" << std::endl;
        upload_file(path);
    } else if (fs::is_directory(m_conf.path / path)) {
        m_api->upload_dir(path);
        for (const auto& entry: fs::recursive_directory_iterator(m_conf.path / path)) {
            fs::path truncated_path = truncate_path(entry.path(), m_conf.path);
            if (entry.is_regular_file()) {
                std::osyncstream(std::cout) << "File " << path << " was added, uploading it to server" << std::endl;
                upload_file(truncated_path);
            } else if (entry.is_directory()) {
                m_api->upload_dir(truncated_path);
            }
        }
    }
}

void Worker::file_removed(const fs::path& path) {
    m_api->remove_file(path);
}

void Worker::file_modified(const fs::path& path) {
    if (!fs::exists(m_conf.path / path)) {
        std::osyncstream(std::cout) << "File " << path << " was modified, but now seems like it's gone" << std::endl;
        return;
    }
    if (!m_modified_timer[path]) {
        m_modified_timer[path] = std::make_unique<boost::asio::deadline_timer>(m_io_service, boost::posix_time::seconds{2});
    }
    m_modified_timer[path]->expires_from_now(boost::posix_time::seconds{2});
    m_modified_timer[path]->async_wait([this, path](const boost::system::error_code& err){
        if (err == boost::asio::error::operation_aborted) {
            return;
        }
        if (!fs::is_regular_file(m_conf.path / path)) {
            return;
        }
        std::osyncstream(std::cout) << "File " << path << " was modified" << std::endl;
        const auto entry = DirEntry::from_path(m_conf.path / path, m_conf.path);
        upload_patch(entry);
        m_modified_timer.erase(path);
    });
}

void Worker::upload_file(const fs::path& path) {
    std::ifstream file {m_conf.path / path, std::ios::binary};
    if (!file.is_open()) {
        return;
    }
    std::string buffer;
    buffer.resize(fs::file_size(m_conf.path / path));
    file.read(buffer.data(), buffer.size());
    m_api->upload_file(std::move(buffer), path);
}

void Worker::perform_initial_sync() {
    std::osyncstream(std::cout) << "Performing initial sync for " << m_conf.path << std::endl;
    std::set<DirEntry> local_entries = extract_entries_from_path(m_conf.path);
    std::osyncstream(std::cout) << "After scanning input dir following entries were found: " << std::endl;
    for (const auto& entry: local_entries) {
        std::osyncstream(std::cout) << "path: " << entry.path << " hash: " << entry.hash << " type: " << entry.type_str() << std::endl;
    }
    m_api->get_files_description([local_entries, this](std::set<DirEntry> remote_entries) {
        download_missing(local_entries, remote_entries);
        upload_missing(local_entries, remote_entries);
        apply_patches(local_entries, remote_entries);
    });
}

void Worker::upload_missing(const std::set<DirEntry>& local_entries, const std::set<DirEntry>& remote_entries) {
    std::set<DirEntry> exists_in_local_not_in_response;
    std::set_difference(local_entries.begin(), local_entries.end(),
                        remote_entries.begin(), remote_entries.end(),
                        std::inserter(exists_in_local_not_in_response, exists_in_local_not_in_response.end()));
    if (exists_in_local_not_in_response.empty()) {
        std::osyncstream(std::cout) << "All local entries presented on server" << std::endl;
    } else {
        std::osyncstream(std::cout) << "new local entries: " << std::endl;
        for (const auto& entry: exists_in_local_not_in_response) {
            std::osyncstream(std::cout) << "path: " << entry.path << " hash: " << entry.hash << " type: " << entry.type_str() << std::endl;
        }
    }
    for (const auto& entry: exists_in_local_not_in_response) {
        if (entry.type == DirEntry::DIR) {
            if (fs::is_empty(m_conf.path / entry.path)) {
                m_api->upload_dir(entry.path);
            }
        } else {
            upload_file(entry.path);
        }
        
    }
}

void Worker::download_missing(const std::set<DirEntry>& local_entries, const std::set<DirEntry>& remote_entries) {
    std::set<DirEntry> exist_in_remote_not_in_local;
    std::set_difference(remote_entries.begin(), remote_entries.end(),
                        local_entries.begin(), local_entries.end(),
                        std::inserter(exist_in_remote_not_in_local, exist_in_remote_not_in_local.end()));
    if (exist_in_remote_not_in_local.empty()) {
        std::osyncstream(std::cout) << "All remote entries presented on local machine" << std::endl;
    } else {
        std::osyncstream(std::cout) << "new remote entries: " << std::endl;
        for (const auto& entry: exist_in_remote_not_in_local) {
            std::osyncstream(std::cout) << "path: " << entry.path << " hash: " << entry.hash << " type: " << entry.type_str() << std::endl;
        }
    }
    for (const auto& entry: exist_in_remote_not_in_local) {
        if (entry.type == DirEntry::DIR) {
            fs::create_directory(m_conf.path / entry.path);
            continue;
        }
        m_api->get_file(entry.path, [this, entry](std::vector<char> data){
            std::ofstream file {m_conf.path / entry.path, std::ios::binary};
            file.write(data.data(), data.size());
        });
    }
}

void Worker::apply_patches(const std::set<DirEntry>& local_entries, const std::set<DirEntry>& remote_entries) {
    std::set<DirEntry> with_different_hash;
    for (const auto& entry: local_entries) {
        if (remote_entries.contains(entry) && remote_entries.find(entry)->hash != entry.hash) {
            with_different_hash.insert(entry);
        }
    }

    if (with_different_hash.empty()) {
        std::osyncstream(std::cout) << "No diff in hashes was found" << std::endl;
    } else {
        std::osyncstream(std::cout) << "Files with different hashes: " << std::endl;
        for (const auto& entry: with_different_hash) {
            std::osyncstream(std::cout) << "path: " << entry.path << " hash: " << entry.hash << " type: " << entry.type_str() << std::endl;
        }
    }

    for (const auto& entry: with_different_hash) {
        upload_patch(entry);
    }
}

void Worker::upload_patch(const DirEntry& entry) {
    m_api->get_meta(entry.path, [entry, this](bool is_file, std::vector<FileChunk> remote_chunks) {
        FILE* stream = fopen((m_conf.path / entry.path).c_str(), "rb");
        if (stream == nullptr) {
            std::osyncstream(std::cerr) << "Upload patch: error in opening " << m_conf.path / entry.path << std::endl;
            return;
        }
        std::string buffer;
        int remote_index = 0;
        while(!feof(stream) && remote_index < remote_chunks.size()) {
            const auto& remote_chunk = remote_chunks[remote_index];
            buffer.resize(remote_chunk.size);
            const auto before_read = ftell(stream);
            const auto bytes_read = fread(buffer.data(), 1, buffer.size(), stream);
            XXH64_hash_t chunk_hash = XXH64(buffer.data(), buffer.size(), 0);
            if (bytes_read < remote_chunk.size) { //read less than remote chunk
                buffer.resize(bytes_read);
                m_api->upload_patch(entry.path, before_read, std::move(buffer), true);
                continue;
            }
            if (chunk_hash != remote_chunk.hash) {
                m_api->upload_patch(entry.path, before_read, std::move(buffer));
            }
            remote_index++;
        }
        if (!feof(stream)) { //read til end
            const auto current_pos = ftell(stream);
            std::string buffer;
            buffer.resize(fs::file_size(m_conf.path / entry.path) - current_pos);
            const auto bytes_read = fread(buffer.data(), 1, buffer.size(), stream);
            if (bytes_read != 0) {
                m_api->upload_patch(entry.path, current_pos, std::move(buffer), true);
            }
        }
    });
}
}