#!/bin/bash

# Get the latest Git commit hash
GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "0")

# Get the current Git branch name
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")

# Get the latest Git tag
GIT_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "0.0.0")

# Remove leading 'v' if present
GIT_TAG_CLEAN=${GIT_TAG#v}

# Extract major, minor, and patch versions
IFS='.' read -r MAJOR MINOR PATCH <<< "${GIT_TAG_CLEAN}"

# Ensure all values are numbers
MAJOR=${MAJOR:-0}
MINOR=${MINOR:-0}
PATCH=${PATCH:-0}

# Output file
OUTPUT_FILE="include/version.h"

cat <<EOL > $OUTPUT_FILE
#pragma once

#define GIT_HASH        F("$GIT_HASH")
#define GIT_BRANCH      F("$GIT_BRANCH")
#define GIT_TAG         F("$GIT_TAG_CLEAN")
#define GIT_TAG_MAJOR   ($MAJOR)
#define GIT_TAG_MINOR   ($MINOR)
#define GIT_TAG_PATCH   ($PATCH)
EOL

echo "Generated $OUTPUT_FILE successfully."
