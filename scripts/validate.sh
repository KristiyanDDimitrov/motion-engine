#!/bin/bash

set -e

./scripts/build.sh
cd build
# Assuming pluginval is installed and in PATH
pluginval --strictness-level 5 --skip-gui-tests --validate ./MotionEngine.vst3