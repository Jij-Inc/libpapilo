#!/bin/bash

cmakefile=$(dirname "$0")/../CMakeLists.txt

# Extract current LIBPAPILO_JIJ_PATCH_VERSION
line_jij_version=$(sed -n '/^set(LIBPAPILO_JIJ_PATCH_VERSION/p' "$cmakefile")
current_version=$(echo "$line_jij_version" | grep -o -E '[0-9]+')

if [ -z "$current_version" ]; then
    echo "Error: Could not find LIBPAPILO_JIJ_PATCH_VERSION in CMakeLists.txt"
    exit 1
fi

new_version=$((current_version + 1))

echo "Incrementing LIBPAPILO_JIJ_PATCH_VERSION from $current_version to $new_version"

# Update LIBPAPILO_JIJ_PATCH_VERSION
sed -i '' "s/^set(LIBPAPILO_JIJ_PATCH_VERSION .*/set(LIBPAPILO_JIJ_PATCH_VERSION $new_version)/" "$cmakefile"
