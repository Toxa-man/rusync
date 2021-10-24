#include "Thread.hpp"
#include <syncstream>
#include <iostream>

namespace rusync {
Thread::Thread(boost::asio::io_service& io_service) : m_io_service{io_service}, work {boost::asio::make_work_guard(io_service)} {
    thread = std::thread([this]() {
        m_io_service.run();
    });
}
Thread::~Thread() {
    boost::asio::post(m_io_service, [this](){work.reset();});
    thread.join();

} 
}