#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

#define PRINTHASH(hash) std::hex << std::setw(8) << std::setfill('0') << hash.first << std::dec

// File hash is a pair of crc32 and file length
typedef std::pair<uint32_t, size_t> filehash_t;

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

int main() {
	std::cerr << "Merkle Tree Builder - Example with 3 hardcoded books\n" << std::endl;

	std::vector<std::string> files = {
		"/data/Ambrose Bierce/An Occurrence at Owl Creek Bridge.md",
		"/data/Ambrose Bierce/The Damned Thing.md",
		"/data/Ambrose Bierce/The Death of Halpin Frayser.md"
	};

	std::vector<filehash_t> hashes;

	std::cerr << "Step 1: Computing CRC32 for each file..." << std::endl;
	for (const auto& file : files) {
		filehash_t hash = compute_file_crc32(file);
		hashes.push_back(hash);
		std::cerr << "  " << file << ": " << PRINTHASH(hash) << std::endl;
	}
	std::cerr << std::endl;

	std::cerr << "Step 2: Building Merkle tree..." << std::endl;
	int level = 1;
	while (hashes.size() > 1) {
		std::cerr << "  Level: " << level << std::endl;
		std::vector<filehash_t> next_level;

		for (size_t i = 0; i < hashes.size(); i += 2) {
			filehash_t hash1 = hashes[i];
			filehash_t hash2 = (i + 1 < hashes.size()) ? hashes[i + 1] : hashes[i];

			filehash_t combined = combine_file_crc32(hash1, hash2);
			next_level.push_back(combined);

			std::cerr << "    Combine " << PRINTHASH(hash1) << " with " << PRINTHASH(hash2)
				<< " => " << PRINTHASH(combined) << std::endl;
		}

		hashes = next_level;
		level++;
	}

	std::cerr << "\nFinal Merkle root hash: " << std::endl << std::flush;
	std::cout << PRINTHASH(hashes[0]) << std::dec << std::endl;

	return 0;
}
