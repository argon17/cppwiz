# Wiz Bulb Control

A lightweight C++ CLI tool to control Wiz Smart Bulbs over UDP.

## Features

- Turn bulbs on/off
- Adjust brightness (0-100%)
- Set RGB colors
- Apply moods/scenes (1-32)
- Simple command-line interface

## Building

```bash
# Release build (optimized)
make release

# Debug build
make debug

# Clean build artifacts
make clean

# Show help
make help
```

Binary: `./build/bin/cppwiz`

## Usage

```bash
# Turn on
./build/bin/cppwiz --ip 192.168.0.100 --on

# Turn off
./build/bin/cppwiz --ip 192.168.0.100 --off

# Set brightness
./build/bin/cppwiz --ip 192.168.0.100 --brightness 75

# Set color (RGB)
./build/bin/cppwiz --ip 192.168.0.100 --color 255,0,0

# Set mood
./build/bin/cppwiz --ip 192.168.0.100 --mood 5

# Show help
./build/bin/cppwiz --help
```

## Options

- `--ip <address>` - Bulb IP address (required)
- `--on` - Turn bulb ON
- `--off` - Turn bulb OFF
- `--brightness <0-100>` - Set brightness
- `--color <r,g,b>` - Set RGB color (0-255 each)
- `--mood <1-32>` - Set mood/scene
- `--help` - Show help message

## Requirements

- C++17 or later
- Linux/Unix with socket support
- GCC or compatible compiler

## License

MIT
