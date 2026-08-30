#!/bin/bash

set -e

echo "Validating MotionEngine..."

# Build the plugin first
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Find the built VST3 plugin
PLUGIN_PATH="build/MotionEngine.vst3"
if [ ! -f "$PLUGIN_PATH" ]; then
    echo "Error: Plugin not found at $PLUGIN_PATH"
    exit 1
fi

echo "Plugin found at $PLUGIN_PATH"

# Run pluginval validation (assuming pluginval is installed and in PATH)
pluginval --strictness-level 5 --skip-gui-tests --validate "$PLUGIN_PATH"

echo "Validation completed successfully!"