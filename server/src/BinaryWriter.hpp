#pragma once

#include <stddef.h>
#include <stdexcept>
#include <string.h>

namespace rusync {

/**
 * @brief Conviniet class for writing object of different types to some binary buffer
 * 
 */
class BinaryWriter {
public:
    BinaryWriter(unsigned char* data, size_t length) : 
        m_data(data),
        m_length{length}
    {

    }
    template <typename T>
    void write(T value) {
        if (m_pos + sizeof(T) > m_length) {
            throw std::out_of_range{"Out of range"};
        }
        unsigned char* current = m_data + m_pos;
        memcpy(current, &value, sizeof(T));
        m_pos += sizeof(T);
    }

    void write(const unsigned char* data, size_t length) {
        if (m_pos + length > m_length) {
            throw std::out_of_range{"Out of range"};
        }
        unsigned char* current = m_data + m_pos;
        memcpy(current, data, length);
        m_pos += length;
    }

    unsigned char* get_current() const {
        return m_data + m_pos;
    }

    size_t get_bytes_remain() const {
        return m_length - m_pos;
    }

private:
    unsigned char* m_data;
    size_t m_length;
    size_t m_pos = 0;
};

}