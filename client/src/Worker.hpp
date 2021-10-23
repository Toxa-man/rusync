#pragma once
#include "Config.hpp"
#include "ServerAPI.hpp"
#include "Thread.hpp"
#include <set>
#include <boost/asio.hpp>
#include <DirEntry.hpp>
#include <map>

namespace rusync {

class ServerAPI;

namespace fs = std::filesystem;

/**
 * @brief Basic unit which perform all main operations on files
 * 
 */
class Worker {
public:
    Worker(const Config& conf);
    ~Worker();

    /**
     * @brief List of Worker operations
     * 
     */
    enum Operation {INITIAL_SYNC, ADDED, REMOVED, MODIFIED};

    /**
     * @brief Dispatched operation args to specific handler
     * 
     * @tparam operation 
     * @tparam Args 
     * @param args 
     */
    template <Operation operation, typename ... Args>
    void perform_operation(Args... args);

private:
    void file_added(const fs::path& path);
    void file_removed(const fs::path& path);
    void file_modified(const fs::path& path);
    void upload_file(const fs::path& path);
    void perform_initial_sync();
    void upload_missing(const std::set<DirEntry>& local_entries, const std::set<DirEntry>& remote_entries);
    void download_missing(const std::set<DirEntry>& local_entries, const std::set<DirEntry>& remote_entries);
    void apply_patches(const std::set<DirEntry>& local_entries, const std::set<DirEntry>& remote_entries);
    void upload_patch(const DirEntry& entry);

    Config m_conf;
    boost::asio::io_service m_io_service;
    std::unique_ptr<ServerAPI> m_api;
    Thread m_thread;
    /**
     * @brief list of deffered timers for operations wihch was queued while client was disconnected from server
     * 
     */
    std::vector<std::shared_ptr<boost::asio::deadline_timer>> m_reschedule_timers;

    /**
     * @brief list of deffered timers for modify operation
     * 
     */
    std::map<fs::path, std::unique_ptr<boost::asio::deadline_timer>> m_modified_timer;
    std::unique_ptr<boost::asio::deadline_timer> m_sync_timer;
};

template <Worker::Operation operation, typename ... Args>
void Worker::perform_operation(Args... args) {
    boost::asio::post(m_io_service, [this, ...args = std::move(args)]() {
        if (!m_api) {
            m_api = std::make_unique<ServerAPI>(m_conf, m_io_service);
        }
        if (!m_api->connected()) {
            auto reschedule_timer = std::make_shared<boost::asio::deadline_timer>(m_io_service, boost::posix_time::seconds(2));
            reschedule_timer->async_wait([this, ...args = std::move(args), reschedule_timer](const boost::system::error_code& /*e*/) {
                perform_operation<operation>(std::move(args)...);
                m_reschedule_timers.erase(std::remove(m_reschedule_timers.begin(), m_reschedule_timers.end(), reschedule_timer));
            });
            m_reschedule_timers.push_back(reschedule_timer);
            return;
        }
        if constexpr (operation == INITIAL_SYNC) {
            perform_initial_sync();
        } else if constexpr (operation == ADDED) {
            file_added(args...);
        } else if constexpr (operation == REMOVED) {
            file_removed(args...);
        } else if constexpr (operation == MODIFIED) {
            file_modified(args...);
        }
    });
}
}