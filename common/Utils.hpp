#pragma once
#include <functional>
#include <memory>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace rusync {

namespace fs = std::filesystem;

/**
 * @brief Convinient function which accumulates data from http req/resp and passes it as vector<char> to provided cb
 * 
 * @tparam ReqT 
 * @param cb 
 * @param req 
 */
template <typename ReqT>
void on_full_data(std::function<void(std::vector<char>)> cb, const ReqT &req) {
    auto accumulated_data = std::make_shared<std::vector<char>>();
    req.on_data([accumulated_data, cb](const uint8_t* data, size_t len) {
        if (len == 0) {
            cb(std::move(*accumulated_data.get()));
            return;
        }
        accumulated_data->resize(accumulated_data->size() + len);
        memcpy(accumulated_data->data() + accumulated_data->size() - len, data, len);
    });
}

/**
 * @brief erases origin from path
 * 
 * @param path 
 * @param origin 
 * @return fs::path truncated path 
 */
inline fs::path truncate_path(fs::path path, fs::path origin) {
    fs::path truncated_path;
    auto path_it = path.begin();
    for (auto origin_it = origin.begin(); origin_it != origin.end(); origin_it++, path_it++);

    for (; path_it != path.end(); path_it++) {
        truncated_path /= *path_it;
    }
    return truncated_path;
} 


/**
 * @brief percent-encode provided url
 * 
 * @param value 
 * @return std::string encoded 
 */
inline std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}
}