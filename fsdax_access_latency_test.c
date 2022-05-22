#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <stdint.h>
#include <x86intrin.h>
#include <sys/time.h>
#include <time.h>

#include "json_parse.h"

#define CACHE_BUFFER_SIZE (36ul<<20)

static const char* config_file_path = "/home/dsb/nvdimm_test/config.json";
struct nvdimm_config config;

static void
read_seq(char* addr){
    uint64_t i;
    char dst_8[8];
    char dst_128[128];
    char* dst_huge;
    uint32_t stride;
    uint32_t loop_num;
    
    loop_num = config.loop_num;
    addr += config.offset;
    stride = config.stride;

    if(config.element_size <= 8){
        for(i=0; i<loop_num; i++){
            dst_8[0] = addr[0];
            dst_8[1] = addr[1];
            dst_8[2] = addr[2];
            dst_8[3] = addr[3];
            dst_8[0] = addr[4];
            dst_8[1] = addr[5];
            dst_8[2] = addr[6];
            dst_8[3] = addr[7];
            addr += stride;
        }
    }
}

static void
read_rnd(char* addr){
    uint64_t i;
    uint64_t dst_8[8];
    char dst_128[128];
    char* dst_huge;
    uint64_t stride;
    uint32_t loop_num;
    char* baseaddr;
    
    // offset = config.offset;
    loop_num = config.loop_num;
    baseaddr = addr + config.offset;
    stride = config.stride;

    if(config.element_size <= 8){
        for(i=0; i<loop_num; i++){
            dst_8[0] = (uint64_t)addr[0];
            dst_8[1] = (uint64_t)addr[1];
            dst_8[2] = (uint64_t)addr[2];
            dst_8[3] = (uint64_t)addr[3];
            dst_8[0] = (uint64_t)addr[4];
            dst_8[1] = (uint64_t)addr[5];
            dst_8[2] = (uint64_t)addr[6];
            dst_8[3] = (uint64_t)addr[7];
            
            addr = baseaddr + (rand()%loop_num)*stride;
            // printf("%p ",addr);
        }
    }
}

static void
write_seq(char* addr){
    uint64_t i;
    char dst_8[8];
    char dst_128[128];
    char* dst_huge;
    uint32_t stride;
    uint32_t loop_num;
    
    loop_num = config.loop_num;
    addr += config.offset;
    stride = config.stride;

    if(config.element_size <= 8){
        for(i=0; i<loop_num; i++){
            addr[0] = 'c';
            _mm_clflushopt((void*)addr);
            _mm_sfence();
            addr += stride;
        }
    }
}

static void
write_rnd(char* addr){
    uint64_t i;
    uint64_t dst_8[8];
    char dst_128[128];
    char* dst_huge;
    uint64_t stride;
    uint32_t loop_num;
    char* baseaddr;
    
    // offset = config.offset;
    loop_num = config.loop_num;
    baseaddr = addr + config.offset;
    stride = config.stride;

    if(config.element_size <= 8){
        for(i=0; i<loop_num; i++){
            addr[0] = 'c';
            _mm_clflushopt((void*)addr);
            _mm_sfence();
            
            addr = baseaddr + (rand()%loop_num)*stride;
            // printf("%p ",addr);
        }
    }
}

static int
access_memory(char* addr){
    switch(config.type){
        case READ_SEQUENTIAL:
            read_seq((char*)addr);
            break;
        case READ_RANDOM:
            read_rnd((char*)addr);
            break;
        case WRITE_SEQUENTIAL:
            write_seq((char*)addr);
            break;
        case WRITE_RANDOM:
            write_rnd((char*)addr);
            break;
        default:
            perror("should not be here");
    }

    return 0;
}

static void
preload_pagetable(char* pmemaddr, size_t mapped_len){
    // 4KB page size
    for(size_t i=0; i < (mapped_len>>12); i ++){
        pmemaddr[i<<12] = 'a';
    }

    return ;
}

static void
clear_cache(){
    char *cache_buffer;

    cache_buffer = (char *) malloc(CACHE_BUFFER_SIZE);
    for(size_t i=0; i < CACHE_BUFFER_SIZE; i ++){
        cache_buffer[i] = '0';
    }
    free(cache_buffer);
}

static int 
test_start(char* addr){
    struct timeval start;
    struct timeval end;
    unsigned long diff;

    // if(config.preload_pagetable){
    //     preload_pagetable(addr, config.pmem_space_size);
    // }

    _mm_mfence();
    gettimeofday(&start, NULL);

    access_memory(addr);
    
    gettimeofday(&end, NULL);
    diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    
    printf("Total time is %ld us\n", diff);
    printf("Latency is %ld ns\n", diff * 1000 / config.loop_num);

    return 0;
}

static char* 
pmem_init(){
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem = 0;

    if ((pmemaddr = (char *)pmem_map_file(config.file_path, config.pmem_space_size, 0,
				0666, &mapped_len, &is_pmem)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}

    if(!is_pmem){
        perror("is not pmem");
        exit(1);
    }

    printf("mmap size is %ld MB\n", mapped_len>>20);

    return pmemaddr;
}

int main(int argc, char** argv){
    parse_json(config_file_path ,&config);
    char* addr = pmem_init();

    clear_cache();
    test_start(addr);

    return 0;
}