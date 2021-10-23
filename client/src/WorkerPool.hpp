#pragma once
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>
#include "Worker.hpp"


namespace rusync {

namespace fs = std::filesystem;
class WorkerPool {
public:
    /**
     * @brief Construct a new Worker Pool object of size size
     * 
     * @param conf 
     * @param size 
     */
    WorkerPool(const Config& conf, unsigned size) {
        for (unsigned i = 0; i < size; i++) {
            m_workers.push_back(std::make_unique<Worker>(conf));
        }
    }

    /**
     * @brief post operation to worker. if request contains path as first parameter - operations for same files will be always perfromed by the same worker.<br>
     * Otherwise - round robin is used to balance load
     * 
     * @tparam operation 
     * @tparam Args 
     * @param args 
     */
    template<Worker::Operation operation, typename ... Args>
    void post_operation(Args... args) {
        if constexpr (sizeof...(args) > 0) {
            const size_t hash = [](const fs::path& path, auto&&...) {
                return fs::hash_value(path);
            }(args...);
            m_workers[hash % m_workers.size()]->perform_operation<operation>(std::move(args)...);
            return;
        }
        m_workers[m_current_worker_index % m_workers.size()]->perform_operation<operation>(std::move(args)...);
        m_current_worker_index++;
    }
private:
    std::vector<std::unique_ptr<Worker>> m_workers;
    std::atomic<uint32_t> m_current_worker_index = 0;
};
}