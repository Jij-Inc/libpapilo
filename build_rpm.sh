#!/bin/bash
# Script to build RPM package for libpapilo

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building libpapilo RPM package...${NC}"

# Create build directory
BUILD_DIR="build/rpm"
rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cd ${BUILD_DIR}
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_SHARED_LIBS=ON

# Build the library
echo -e "${YELLOW}Building libpapilo...${NC}"
make -j$(nproc) libpapilo

# Create RPM package
echo -e "${YELLOW}Creating RPM package...${NC}"
cpack -G RPM

# List generated packages
echo -e "${GREEN}Generated RPM packages:${NC}"
ls -la *.rpm

echo -e "${GREEN}RPM package build complete!${NC}"
echo -e "${GREEN}To install: sudo rpm -ivh libpapilo-*.rpm${NC}"
echo -e "${GREEN}To uninstall: sudo rpm -e libpapilo${NC}"