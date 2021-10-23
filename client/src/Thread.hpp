#pragma once
#include <thread>
#include <boost/asio.hpp>

namespace rusync {
struct Thread {
    Thread(boost::asio::io_service& io_service);
    ~Thread();
    std::thread thread;
    /**
     * @brief work object which prevents asio event loop from exiting
     * 
     */
    boost::asio::executor_work_guard<boost::asio::io_service::executor_type> work;
    boost::asio::io_service& m_io_service;
};
}