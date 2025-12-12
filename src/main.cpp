#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <zlib.h>
#include <filesystem>
#include <algorithm> 
#include <mutex>

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

	while (file.read(buffer, sizeof(buffer))) {
		crc = crc32(crc, reinterpret_cast<Bytef*>(buffer), file.gcount());
	}

	if (file.gcount() > 0) {
		crc = crc32(crc, reinterpret_cast<Bytef*>(buffer), file.gcount());
	}

	return std::make_pair(crc, file.gcount());
}

filehash_t combine_file_crc32(const filehash_t &hash1, const filehash_t &hash2) {
	uint32_t crc_combined = crc32_combine(hash1.first, hash2.first, hash2.second);
	size_t len_combined = hash1.second + hash2.second;

	return std::make_pair(crc_combined, len_combined);
}

void combine_crc32(const std::vector<filehash_t>& hashes, std::vector<filehash_t>& next_level, std::mutex& mtx, const int i)
{
	
	filehash_t hash1 = hashes[i];
	filehash_t hash2 = (i + 1 < hashes.size()) ? hashes[i + 1] : hashes[i];

	filehash_t combined = combine_file_crc32(hash1, hash2);
	next_level[i/2] = (combined);
	
	std::lock_guard<std::mutex> lock(mtx);
	DEBUG_LOG("    Combine " << PRINTHASH(hash1) << " with " << PRINTHASH(hash2)
		<< " => " << PRINTHASH(combined));
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
    

	std::vector<filehash_t> hashes{files.size()};
	std::vector<std::thread> threads{};

	std::mutex mtx;
	DEBUG_LOG("Step 1: Computing CRC32 for each file...");
	for (int i = 0; i < files.size(); ++i) {
		auto& file = files[i];
		threads.push_back(
			std::thread ([i, &hashes, &file, &mtx](){
				filehash_t hash = compute_file_crc32(file);
				hashes[i] = hash;
				std::lock_guard<std::mutex> lock(mtx);
				DEBUG_LOG("  " << file << ": " << PRINTHASH(hash));
		}));
	}
	DEBUG_LOG("");

	for (auto& t: threads)
	{
		t.join();
	}

	DEBUG_LOG("Step 2: Building Merkle tree...");
	int level = 1;
	while (hashes.size() > 1) {
		DEBUG_LOG("  Level: " << level);
		std::vector<filehash_t> next_level{(hashes.size()+1) / 2};
		std::vector<std::thread> threads{};
		for (size_t i = 0; i < hashes.size(); i += 2) {
			threads.push_back(std::thread([&hashes, &next_level, &mtx, i](){
				combine_crc32(hashes, next_level, mtx, i);
			}));
		}

		for (auto& t: threads)
		{
			t.join();
		}
		threads.clear();

		hashes = next_level;
		level++;
	}

	DEBUG_LOG("\nFinal Merkle root hash: " << std::endl << std::flush);
	std::cout << PRINTHASH(hashes[0]) << std::dec << std::endl;

	return 0;
}
