#pragma once

#include <string>
#include <mutex>
#include <vector>

#include "typedefs.h"

class Task
{
public:
    virtual ~Task() = default;
    virtual void operator()() = 0;
};

filehash_t compute_file_crc32(const std::string& filepath);
filehash_t combine_file_crc32(const filehash_t &hash1, const filehash_t &hash2);

class ComputeCrcTask final : public Task 
{
public:
    ComputeCrcTask(std::string file, std::mutex& debug_mutex, std::vector<filehash_t>& hashes, int i):
        _file(file), 
        _debug_mutex(debug_mutex), 
        _hashes(hashes), 
        _i(i) {};
    ~ComputeCrcTask() override = default;
    void operator()() override;

private:
    std::string _file;
    std::mutex& _debug_mutex;
    std::vector<filehash_t>& _hashes;
    int _i;
};

class CombineCrcTask final : public Task 
{
public:
    CombineCrcTask(std::mutex& debug_mutex, std::vector<filehash_t>& hashes, std::vector<filehash_t>& next_level,  int i):
        _debug_mutex(debug_mutex), 
        _hashes(hashes), 
        _next_level(next_level),
        _i(i) {};
    ~CombineCrcTask() override = default;
    void operator()() override;

private:
    std::string _file;
    std::mutex& _debug_mutex;
    std::vector<filehash_t>& _hashes;
    std::vector<filehash_t>& _next_level;
    long unsigned int _i;
};
