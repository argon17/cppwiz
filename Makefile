CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic
RELEASE_FLAGS := -O2 -DNDEBUG
DEBUG_FLAGS := -g -O0 -DDEBUG

SRC := cppwiz.cpp
BIN_DIR := build/bin
TARGET := $(BIN_DIR)/cppwiz

.PHONY: all release debug clean help

all: release

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)
	@echo "✓ Release build complete: $(TARGET)"

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)
	@echo "✓ Debug build complete: $(TARGET)"

$(TARGET): $(SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

run: release
	./$(TARGET) --help

clean:
	rm -rf build
	@echo "✓ Cleaned build artifacts"

help:
	@echo "Wiz Bulb Control - C++ Build"
	@echo ""
	@echo "Targets:"
	@echo "  make release       Build optimized release binary"
	@echo "  make debug         Build debug binary with symbols"
	@echo "  make clean         Remove build artifacts"
	@echo "  make run           Build and show help"
	@echo "  make help          Show this help message"
	@echo ""
	@echo "Output: $(TARGET)"
