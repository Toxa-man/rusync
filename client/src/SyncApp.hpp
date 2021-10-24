#pragma once
#include "Config.hpp"
#include "WorkerPool.hpp"
#include "efsw/efsw.hpp"
#include <syncstream>
#include <boost/asio.hpp>


namespace rusync {

inline std::atomic_bool stopped = false;

/**
 * @brief Main class responsible for application logic
 * 
 */
class SyncApp : efsw::FileWatchListener {
public:
    SyncApp(const Config& config);

    /**
     * @brief perform full sync between client and server
     * 
     */
    void full_sync();

    /**
     * @brief Called when new event occured within watched client folder
     * 
     * @param watchid 
     * @param dir 
     * @param filename 
     * @param action 
     * @param oldFilename 
     */
    void handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename ) override;
private:
    Config m_conf;
    WorkerPool m_worker_pool;
    std::unique_ptr<efsw::FileWatcher> m_file_watcher;
    boost::asio::io_service m_service;
    boost::asio::deadline_timer m_resync_timer;

};
}