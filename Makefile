# Makefile for Merkle Tree Builder

.PHONY: all build clean docker-build docker-run

override PROJECT_ROOT := $(shell sh -c "git rev-parse --git-dir | xargs dirname")

# Default target
all: docker-run

# Build the application
build:
	cmake -S ${PROJECT_ROOT}/src -B ${PROJECT_ROOT}/build
	cmake --build ${PROJECT_ROOT}/build

# Clean build artifacts
clean:
	rm -rf ${PROJECT_ROOT}/build

# Build Docker image
docker-build:
	docker build -t merkle-tree .

# Run in Docker container
docker-run: docker-build
	docker run --rm -v ${PROJECT_ROOT}/data:/data merkle-tree
