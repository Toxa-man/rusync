#include "Thread.hpp"
#include <syncstream>
#include <iostream>

namespace rusync {
Thread::Thread(boost::asio::io_service& io_service) : m_io_service{io_service}, work {boost::asio::make_work_guard(io_service)} {
    thread = std::thread([this]() {
        std::osyncstream(std::cout) << "running worker thread " << std::endl;
        m_io_service.run();
        std::osyncstream(std::cout) << "no more work to do, stopped worker thread " << std::endl;
    });
}
Thread::~Thread() {
    boost::asio::post(m_io_service, [this](){work.reset();});
    thread.join();
    std::osyncstream(std::cout) << "~Thread()" << std::endl;

} 
}