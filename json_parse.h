#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <cjson/cJSON.h>

#define GB(x) (x<<30)
#define MB(x) (x<<20)
#define KB(x) (x<<10)

enum test_op_type {
    INVALID_TYPE ,
    WRITE_RANDOM ,
    WRITE_SEQUENTIAL,
    READ_RANDOM,
    READ_SEQUENTIAL,
};


struct nvdimm_config {
    enum test_op_type type;
    size_t pmem_space_size;
    size_t element_size;
    uint64_t loop_num;

    uint32_t stride;
    uint32_t offset;
    uint32_t repetition;

    char file_path[128];
    bool preload_pagetable;
};

int parse_json(const char *path, struct nvdimm_config *config);