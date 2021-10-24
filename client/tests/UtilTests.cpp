#include <gtest/gtest.h>
#include "Utils.hpp"

TEST(EncodeUrl, basic_encode_test) {
    std::string source = "key=abc&path=test file.cpp";
    std::string encoded = rusync::url_encode(source);
    EXPECT_TRUE(encoded == "key%3Dabc%26path%3Dtest%20file.cpp");
}