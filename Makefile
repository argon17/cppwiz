CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic
RELEASE_FLAGS := -O2 -DNDEBUG
DEBUG_FLAGS := -g -O0 -DDEBUG
LDFLAGS :=

SRC := cppwiz.cpp
BIN_DIR := build/bin

# Detect Windows (MinGW / MSYS2)
ifeq ($(OS),Windows_NT)
  TARGET := $(BIN_DIR)/cppwiz.exe
  LDFLAGS += -lws2_32
  MKDIR = if not exist "$(subst /,\,$(BIN_DIR))" mkdir "$(subst /,\,$(BIN_DIR))"
  RM = if exist build rmdir /s /q build
else
  TARGET := $(BIN_DIR)/cppwiz
  MKDIR = mkdir -p $(BIN_DIR)
  RM = rm -rf build
endif

.PHONY: all release debug clean help

all: release

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)
	@echo "✓ Release build complete: $(TARGET)"

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)
	@echo "✓ Debug build complete: $(TARGET)"

$(TARGET): $(SRC)
	@$(MKDIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

run: release
	./$(TARGET) --help

clean:
	@$(RM)
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
