#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>
#include <algorithm> 
#include <mutex>
#include <filesystem>

#include "typedefs.h"
#include "task.h"
#include "thread_pool.h"

namespace fs = std::filesystem;

static int NUMBER_OF_THREADS = 12;

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
    
	DEBUG_LOG("Step 1: Computing CRC32 for each file...");

	std::vector<filehash_t> hashes{files.size()};
	std::mutex mtx;
	ThreadPool pool(NUMBER_OF_THREADS);
	
	for (int i = 0; i < files.size(); ++i) {
		auto& file = files[i];
		pool.submit_task(std::make_unique<ComputeCrcTask>(file, mtx, hashes, i));
	}
	pool.wait_until_empty();
	DEBUG_LOG("");

	DEBUG_LOG("Step 2: Building Merkle tree...");

	int level = 1;
	while (hashes.size() > 1) {
		DEBUG_LOG("  Level: " << level);
		std::vector<filehash_t> next_level{(hashes.size()+1) / 2};
		for (size_t i = 0; i < hashes.size(); i += 2) {
			pool.submit_task(std::make_unique<CombineCrcTask>(mtx, hashes, next_level, i));
		}
		pool.wait_until_empty();
		hashes = next_level;
		level++;
	}

	DEBUG_LOG("\nFinal Merkle root hash: " << std::endl << std::flush);
	std::cout << PRINTHASH(hashes[0]) << std::dec << std::endl;

	return 0;
}
