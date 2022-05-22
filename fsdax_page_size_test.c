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

#define PMEM_LEN 4ul<<30

static const char* path = "/mnt/pmem/file";

size_t virtual_to_physical(size_t addr)
{
    printf("addr %p\n", addr);
    int fd = open("/proc/self/pagemap", O_RDONLY);
    if(fd < 0)
    {
        printf("open '/proc/self/pagemap' failed!\n");
        return 0;
    }
    size_t pagesize = getpagesize();
    size_t offset = (addr / pagesize) * sizeof(uint64_t);
    if(lseek(fd, offset, SEEK_SET) < 0)
    {
        printf("lseek() failed!\n");
        close(fd);
        return 0;
    }
    uint64_t info;
    if(read(fd, &info, sizeof(uint64_t)) != sizeof(uint64_t))
    {
        printf("read() failed!\n");
        close(fd);
        return 0;
    }
    if((info & (((uint64_t)1) << 63)) == 0)
    {
        printf("page is not present!\n");
        close(fd);
        return 0;
    }
    size_t frame = info & ((((uint64_t)1) << 55) - 1);
    size_t phy = frame * pagesize + addr % pagesize;

    printf("self pagemap phy %lx\n", phy);
    close(fd);
    return phy;
}

static int
access_memory(char *addr){
    char buf[16] = {'c'};
    pmem_memcpy_persist(addr, buf, 16);

    return 0;
}

static char* 
pmem_init(){
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem = 0;

    if ((pmemaddr = (char *)pmem_map_file(path, 0, 0,
				0666, &mapped_len, &is_pmem)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}

    printf("is pmem %d\n", is_pmem);
    // if(!is_pmem){
    //     printf("fail to map fsdax, is not pmem\n");
    //     return 0;
    // }
    
    printf("mmap size is %ld MB\n", mapped_len>>20);

    return pmemaddr;
}

int main(int argc, char** argv){
    char* addr = pmem_init();

    access_memory(addr);
    virtual_to_physical((size_t)addr);
    virtual_to_physical((size_t)addr + (4<<10));

    return 0;
}