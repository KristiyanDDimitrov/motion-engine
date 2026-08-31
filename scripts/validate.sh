#!/bin/bash

set -e

echo "Validating MotionEngine..."

# Build the plugin first
./scripts/build.sh

# Validate using pluginval
echo "Validating with pluginval..."
pluginval --strictness-level 5 --skip-gui-tests --validate build/MotionEngine.vst3

echo "Validation completed successfully."