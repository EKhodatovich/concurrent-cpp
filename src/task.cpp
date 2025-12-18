#include "task.h"
#include <fstream>
#include <iostream>
#include <zlib.h>
#include "typedefs.h"


filehash_t compute_file_crc32(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::binary);
	if (!file) {
		std::cerr << "Error opening file: " << filepath << std::endl;
		std::exit(1);
	}

	uint32_t crc = crc32(0L, Z_NULL, 0);
	char buffer[4096];
	uint32_t total_len = 0; 
	
	while (!file.eof()) {
		file.read(buffer, sizeof(buffer));
	    uint32_t current_len = file.gcount();
	    crc = crc32(crc, reinterpret_cast<Bytef*>(buffer), current_len);
	    total_len += current_len;
	}

	
	return std::make_pair(crc, total_len);
}

filehash_t combine_file_crc32(const filehash_t &hash1, const filehash_t &hash2) {
	uint32_t crc_combined = crc32_combine(hash1.first, hash2.first, hash2.second);
	size_t len_combined = hash1.second + hash2.second;

	return std::make_pair(crc_combined, len_combined);
}

void ComputeCrcTask::operator()()
{
    filehash_t hash = compute_file_crc32(_file);
    _hashes[_i] = hash; 
    std::lock_guard<std::mutex> lock(_debug_mutex);
    DEBUG_LOG("  " << _file << ": " << PRINTHASH(hash));
}

void CombineCrcTask::operator()()
{
	filehash_t hash1 = _hashes[_i];
	filehash_t hash2 = (_i + 1 < _hashes.size()) ? _hashes[_i + 1] : _hashes[_i];

	filehash_t combined = combine_file_crc32(hash1, hash2);
	_next_level[_i/2] = (combined);
	
	std::lock_guard<std::mutex> lock(_debug_mutex);
	DEBUG_LOG("    Combine " << PRINTHASH(hash1) << " with " << PRINTHASH(hash2)
		<< " => " << PRINTHASH(combined));
}