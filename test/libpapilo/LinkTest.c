/**
 * @file LinkTest.c
 * @brief Simple C program to test that libpapilo can be linked and used
 *
 * This test verifies that the libpapilo shared library is properly installed
 * and can be linked against from a C program. It calls basic libpapilo
 * functions to ensure the library is functional.
 */

#include <libpapilo.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Test basic library functionality
    const char* version = libpapilo_version();
    if (version == NULL) {
        fprintf(stderr, "Error: libpapilo_version() returned NULL\n");
        return 1;
    }
    
    printf("libpapilo version: %s\n", version);
    
    // Test problem builder creation/cleanup to verify library linkage
    libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
    if (builder == NULL) {
        fprintf(stderr, "Error: Failed to create problem builder\n");
        return 1;
    }
    
    printf("Successfully created problem builder\n");
    
    // Clean up
    libpapilo_problem_builder_free(builder);
    
    printf("Link test passed: libpapilo is properly installed and functional\n");
    return 0;
}