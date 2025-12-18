#include <atomic>
#include <bits/types/struct_timeval.h>
#include <cerrno>
#include <cstddef>
#include <event2/event.h>
#include <event2/util.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <zlib.h>
#include <algorithm> 
#include <mutex>
#include <filesystem>
#include <fcntl.h>
#include "event2/event.h"
#include "event2/event_struct.h"
#include <sys/time.h>

#include "typedefs.h"
#include "task.h"
#include "thread_pool.h"

namespace fs = std::filesystem;

static int NUMBER_OF_THREADS = 12;

struct event_context
{
	std::string filepath;
	event* ev;
	std::vector<filehash_t>* hashes;
	int hash_index;
	std::atomic<int>* remaining_files;
	char buffer[4096];
	uint32_t crc = crc32(0L, Z_NULL, 0);
	size_t len = 0;
};

void process_file_callback(evutil_socket_t fd, short events, void *arg)
{
	struct event_context* ctx = (struct event_context*)arg;
	ssize_t n = read(fd, ctx->buffer, std::size(ctx->buffer));
	if (n > 0)
	{
		ctx->crc = crc32(ctx->crc, reinterpret_cast<Bytef*>(ctx->buffer), n);
		ctx->len += n;
	}
	else if (n == 0) {
		(*ctx->hashes)[ctx->hash_index] = std::make_pair(ctx->crc, ctx->len);
		--(*ctx->remaining_files);
		event_del(ctx->ev);
		event_free(ctx->ev);
		close(fd);
		delete ctx;
	}
	else {
		if (errno == EAGAIN) return;

		std::cerr << "Error reading file: " << hstrerror(errno) << std::endl;
		--(*ctx->remaining_files);
		event_del(ctx->ev);
		event_free(ctx->ev);
		close(fd);
		delete ctx;
	}
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
    
	DEBUG_LOG("Step 1: Computing CRC32 for each file...");

	std::vector<filehash_t> hashes{files.size()};
	
	// Создаём конфигурацию для event_base
	struct event_config *cfg = event_config_new();
	// Запрещаем использование epoll
	event_config_avoid_method(cfg, "epoll");
	// Создаём event_base с этой конфигурацией
	struct event_base *base = event_base_new_with_config(cfg);
	event_config_free(cfg);

	if (!base) {
		std::cerr << "Failed to create event_base" << std::endl;
		return 1;
	}
	
	std::atomic<int> remaining_files{static_cast<int>(files.size())};

	for (size_t i = 0; i < files.size(); ++i)
	{
		struct event_context* ctx = new event_context();
		evutil_socket_t fd = open(files[i].c_str(), O_RDONLY | O_NONBLOCK);
		if (fd == -1)
		{
			std::cerr << "Failed to open file: " << files[i] << "("<< hstrerror(errno)<< ")" << std::endl;
			--remaining_files;
			delete ctx;
			continue;
		}
		ctx->hashes = &hashes;
		ctx->hash_index = i;
		ctx->remaining_files = &remaining_files;

		struct event *ev = event_new(base, fd, EV_READ | EV_PERSIST, process_file_callback, ctx);
		if (ev == NULL)
		{
			std::cerr << "Failed to create event" << std::endl;
			--remaining_files;
			delete ctx;
			continue;
		}
		ctx->ev = ev;

		if (event_add(ev, NULL) == -1)
		{
			std::cerr << "Failed to add event" << std::endl;
			event_del(ev);
			event_free(ev);
			--remaining_files;
			delete ctx;
			continue;
		}

	}
	
	while (remaining_files > 0)
	{
		event_base_loop(base, EVLOOP_ONCE);
	}
	
	event_base_free(base);

	DEBUG_LOG("");

	DEBUG_LOG("Step 2: Building Merkle tree...");

	std::mutex mtx{};
	ThreadPool pool {NUMBER_OF_THREADS};
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
