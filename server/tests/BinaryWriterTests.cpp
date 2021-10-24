#include <gtest/gtest.h>
#include <cstring>
#include <stdexcept>
#include "BinaryWriter.hpp"

TEST(BinaryWriter, write_uint8) {
    uint8_t value = 15;
    unsigned char buffer[sizeof(value)]{};
    rusync::BinaryWriter writer{buffer, sizeof(buffer)};
    writer.write(value);
    EXPECT_EQ(std::memcmp(buffer, &value, sizeof(value)), 0);
}

TEST(BinaryWriter, write_multiple_values) {
    uint8_t value1 = 15;
    int value2 = 123161234;
    unsigned char buffer[sizeof(value1) + sizeof(value2)]{};
    rusync::BinaryWriter writer{buffer, sizeof(buffer)};
    writer.write(value1);
    writer.write(value2);
    EXPECT_EQ(std::memcmp(buffer, &value1, sizeof(value1)), 0);
    EXPECT_EQ(std::memcmp(buffer + sizeof(value1), &value2, sizeof(value2)), 0);
}

TEST(BinaryWriter, write_buffer) {
    unsigned char source_buffer[] = "abcd";
    unsigned char buffer[sizeof(source_buffer)]{};
    rusync::BinaryWriter writer{buffer, sizeof(buffer)};
    writer.write(source_buffer, sizeof(source_buffer));
    EXPECT_EQ(std::memcmp(buffer, source_buffer, sizeof(source_buffer)), 0);
}

TEST(BinaryWriter, get_remain) {
    uint8_t value1 = 15;
    int value2 = 123161234;
    unsigned char buffer[sizeof(value1) + sizeof(value2)]{};
    rusync::BinaryWriter writer{buffer, sizeof(buffer)};
    writer.write(value1);
    EXPECT_EQ(writer.get_bytes_remain(), sizeof(value2));
    writer.write(value2);
    EXPECT_EQ(std::memcmp(buffer, &value1, sizeof(value1)), 0);
    EXPECT_EQ(std::memcmp(buffer + sizeof(value1), &value2, sizeof(value2)), 0);
}


