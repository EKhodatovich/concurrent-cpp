#pragma once

#include <cstdint>
#include <utility>
#include <iomanip>

#define PRINTHASH(hash) std::hex << std::setw(8) << std::setfill('0') << hash.first << std::dec

#ifdef DEBUG
    #define DEBUG_LOG(msg) std::cerr << "[DEBUG] " << msg << std::endl
#else
    #define DEBUG_LOG(msg) ((void)0)  // No-op when DEBUG is not defined
#endif
// File hash is a pair of crc32 and file length
typedef std::pair<uint32_t, std::size_t> filehash_t;

