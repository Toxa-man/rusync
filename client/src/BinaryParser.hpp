#include <stddef.h>
#include <stdexcept>
#include <string.h>

namespace rusync {

/**
 * @brief Contains methods for convinient extracting of object from binary blob
 * 
 */
class BinaryParser {
public:
    /**
     * @brief Construct a new Binary Parser object
     * 
     * @param data 
     * @param length 
     */
    BinaryParser(const unsigned char* data, size_t length) : 
        m_data(data),
        m_length{length}
    {

    }
    /**
     * @brief reads object of type T
     * 
     * @tparam T - object to read, should be DefaultConstructable
     * @return T - read object
     */
    template <typename T>
    T read() {
        if (m_pos + sizeof(T) > m_length) {
            throw std::out_of_range{"Out of range"};
        }
        const unsigned char* current = m_data + m_pos;
        T res;
        memcpy(&res, current, sizeof(T));
        m_pos += sizeof(T);
        return res;
    }

    /**
     * @brief Get pointer to current position within binary data
     * 
     * @return const unsigned* 
     */
    const unsigned char* get_current() const {
        return m_data + m_pos;
    }

    /**
     * @brief Get remaining bytes of input binary data
     * 
     * @return size_t 
     */
    size_t get_bytes_remain() const {
        return m_length - m_pos;
    }

private:
    const unsigned char* m_data;
    size_t m_length;
    size_t m_pos = 0;
};

}