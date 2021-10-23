#include "SyncApp.hpp"

namespace rusync {
SyncApp::SyncApp(const Config& config) :
    m_conf{config}, 
    m_worker_pool {m_conf, std::max(1u, (unsigned)std::thread::hardware_concurrency())}, 
    m_resync_timer{m_service, boost::posix_time::seconds{10}} {

    m_file_watcher = std::make_unique<efsw::FileWatcher>();
    m_file_watcher->addWatch(config.path, this, true);
    m_file_watcher->watch();
    full_sync();
    m_service.run();
}

void SyncApp::full_sync() {
    if (stopped) {
        m_service.stop();
        return;
    }
    m_worker_pool.post_operation<Worker::INITIAL_SYNC>();
    m_resync_timer.async_wait([this](const boost::system::error_code&){
        m_resync_timer.expires_at(m_resync_timer.expires_at() + boost::posix_time::seconds{10});
        full_sync();
    });
}

void SyncApp::handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename ) {
    switch( action )
    {
    case efsw::Actions::Add:
        std::osyncstream(std::cout) << "DIR (" << dir << ") FILE (" << filename << ") has event Added" << std::endl;
        m_worker_pool.post_operation<Worker::ADDED>(truncate_path(fs::path(dir) / filename, m_conf.path));
        break;
    case efsw::Actions::Delete:
        std::osyncstream(std::cout) << "DIR (" << dir << ") FILE (" << filename << ") has event Delete" << std::endl;
        m_worker_pool.post_operation<Worker::REMOVED>(truncate_path(fs::path(dir) / filename, m_conf.path));
        break;
    case efsw::Actions::Modified:
        std::osyncstream(std::cout) << "DIR (" << dir << ") FILE (" << filename << ") has event Modified" << std::endl;
        m_worker_pool.post_operation<Worker::MODIFIED>(truncate_path(fs::path(dir) / filename, m_conf.path));
        break;
    case efsw::Actions::Moved:
            std::osyncstream(std::cout) << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from (" << oldFilename << ")" << std::endl;
        break;
    default:
        std::osyncstream(std::cout) << "Should never happen!" << std::endl;
    }
}   
}