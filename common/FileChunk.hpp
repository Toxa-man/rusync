#pragma once
#include <cstdint>

struct FileChunk {
    uint32_t size;
    uint64_t hash;
};
