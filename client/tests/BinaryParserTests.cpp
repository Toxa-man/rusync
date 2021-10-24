#include <gtest/gtest.h>
#include <cstring>
#include <stdexcept>
#include "BinaryParser.hpp"

TEST(BinaryParser, read_uint8) {
    uint8_t byte = 15;
    unsigned char buffer[sizeof(byte)];
    std::memcpy(buffer, &byte, sizeof(byte));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(byte, parser.read<uint8_t>());
}

TEST(BinaryParser, read_uint32) {
    uint32_t value = 123522351;
    unsigned char buffer[sizeof(value)];
    std::memcpy(buffer, &value, sizeof(value));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(value, parser.read<uint32_t>());
}


TEST(BinaryParser, read_multiple_values) {
    uint32_t value1 = 123522351;
    char value2 = 'd';
    long long value3 = 43524124234;
    unsigned char buffer[sizeof(value1) + sizeof(value2) + sizeof(value3)];
    std::memcpy(buffer, &value1, sizeof(value1));
    std::memcpy(buffer + sizeof(value1), &value2, sizeof(value2));
    std::memcpy(buffer + sizeof(value1) + sizeof(value2), &value3, sizeof(value3));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(value1, parser.read<uint32_t>());
    ASSERT_EQ(value2, parser.read<char>());
    ASSERT_EQ(value3, parser.read<long long>());
}

TEST(BinaryParser, test_remaing_bytes_zero) {
    uint8_t byte = 15;
    unsigned char buffer[sizeof(byte)];
    std::memcpy(buffer, &byte, sizeof(byte));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(byte, parser.read<uint8_t>());
    ASSERT_EQ(parser.get_bytes_remain(), 0);
}

TEST(BinaryParser, test_remaing_bytes_not_zero) {
    uint32_t value1 = 123522351;
    long long value2 = 43524124234;
    unsigned char buffer[sizeof(value1) + sizeof(value2)];
    std::memcpy(buffer, &value1, sizeof(value1));
    std::memcpy(buffer + sizeof(value1), &value2, sizeof(value2));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(value1, parser.read<uint32_t>());
    ASSERT_EQ(parser.get_bytes_remain(), sizeof(value2));
    ASSERT_EQ(value2, parser.read<long long>());
}

TEST(BinaryParser, test_current_position) {
    uint32_t value1 = 123522351;
    long long value2 = 43524124234;
    unsigned char buffer[sizeof(value1) + sizeof(value2)];
    std::memcpy(buffer, &value1, sizeof(value1));
    std::memcpy(buffer + sizeof(value1), &value2, sizeof(value2));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(value1, parser.read<uint32_t>());
    ASSERT_EQ(parser.get_current(), buffer + sizeof(value1));
    ASSERT_EQ(value2, parser.read<long long>());
}

TEST(BinaryParser, test_throw_out_of_range) {
    uint8_t byte = 15;
    unsigned char buffer[sizeof(byte)];
    std::memcpy(buffer, &byte, sizeof(byte));
    rusync::BinaryParser parser(buffer, sizeof(buffer));
    ASSERT_EQ(byte, parser.read<uint8_t>());
    EXPECT_THROW({
        parser.read<uint8_t>();
    }, std::out_of_range);
}