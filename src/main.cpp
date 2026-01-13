#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>
#include <filesystem>
#include <algorithm> 

#define PRINTHASH(hash) std::hex << std::setw(8) << std::setfill('0') << hash.first << std::dec

#ifdef DEBUG
    #define DEBUG_LOG(msg) std::cerr << "[DEBUG] " << msg << std::endl
#else
    #define DEBUG_LOG(msg) ((void)0)  // No-op when DEBUG is not defined
#endif
// File hash is a pair of crc32 and file length
typedef std::pair<uint32_t, size_t> filehash_t;
namespace fs = std::filesystem;

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

int main() {
	DEBUG_LOG("Merkle Tree Builder - Example with 3 hardcoded books\n");
	
    std::vector<std::string> files;
    std::string data_dir = "/data";
    
    DEBUG_LOG("Step 0: Read directory recursively, collect all files and sort them alphabetically");
    try {
        for (const auto& entry : fs::recursive_directory_iterator(data_dir)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& ex) {
        std::cerr << "Error reading directory: " << ex.what() << std::endl;
        return 1;
    }
    
    std::sort(files.begin(), files.end());
    

	std::vector<filehash_t> hashes;

	DEBUG_LOG("Step 1: Computing CRC32 for each file...");
	for (const auto& file : files) {
		filehash_t hash = compute_file_crc32(file);
		hashes.push_back(hash);
		DEBUG_LOG("  " << file << ": " << PRINTHASH(hash));
	}
	DEBUG_LOG("");

	DEBUG_LOG("Step 2: Building Merkle tree...");
	int level = 1;
	while (hashes.size() > 1) {
		DEBUG_LOG("  Level: " << level);
		std::vector<filehash_t> next_level;

		for (size_t i = 0; i < hashes.size(); i += 2) {
			filehash_t hash1 = hashes[i];
			filehash_t hash2 = (i + 1 < hashes.size()) ? hashes[i + 1] : hashes[i];

			filehash_t combined = combine_file_crc32(hash1, hash2);
			next_level.push_back(combined);

			DEBUG_LOG("    Combine " << PRINTHASH(hash1) << " with " << PRINTHASH(hash2)
				<< " => " << PRINTHASH(combined));
		}

		hashes = next_level;
		level++;
	}

	DEBUG_LOG("\nFinal Merkle root hash: " << std::flush);
	std::cout << PRINTHASH(hashes[0]) << std::dec << std::endl;

	return 0;
}
