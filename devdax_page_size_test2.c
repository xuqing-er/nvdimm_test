#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>

#define IOCTL_CMD_GET_PHYADDR   1234

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
check_page_table(size_t vaddr)
{
    int fd = open("/dev/phyaddr", O_RDWR);
    assert(fd > 0);

    size_t phyaddr = vaddr;

    int ret = ioctl(fd, IOCTL_CMD_GET_PHYADDR, &phyaddr);
    assert(ret == 0);

    printf("phyaddr by /dev/phyaddr: %lx\n", phyaddr);
    return 0;
}

static int
access_memory(char* addr, size_t size, pmem2_persist_fn persist)
{
    char data[size];

    memcpy(addr, data, size);
    persist(addr, size);
}

int
main(int argc, char *argv[])
{
	int fd;
	struct pmem2_config *cfg;
	struct pmem2_map *map;
	struct pmem2_source *src;
    size_t source_algin;
	pmem2_persist_fn persist;

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		exit(1);
	}

	if ((fd = open(argv[1], O_RDWR)) < 0) {
		perror("open");
		exit(1);
	}

	if (pmem2_config_new(&cfg)) {
		pmem2_perror("pmem2_config_new");
		exit(1);
	}

	if (pmem2_source_from_fd(&src, fd)) {
		pmem2_perror("pmem2_source_from_fd");
		exit(1);
	}

    source_algin = 4<<10;
	if (pmem2_source_alignment(src, &source_algin)) {
		pmem2_perror("pmem2_source_alignment");
		exit(1);
	}

    if (pmem2_config_set_required_store_granularity(cfg,
			PMEM2_GRANULARITY_CACHE_LINE)) {
		pmem2_perror("pmem2_config_set_required_store_granularity");
		exit(1);
	}

	if (pmem2_map_new(&map, cfg, src)) {
		pmem2_perror("pmem2_map_new");
		exit(1);
	}

    char *addr = pmem2_map_get_address(map);
	size_t size = pmem2_map_get_size(map);

    printf("addr :%p, size: %ldGB\n", addr, size>>30);

    persist = pmem2_get_persist_fn(map);

    access_memory(addr, 20, persist);
    check_page_table((size_t)addr);
    virtual_to_physical((size_t)addr);
    virtual_to_physical((size_t)addr+(4<<10));
    virtual_to_physical((size_t)addr+(8<<10));
    virtual_to_physical((size_t)addr+(2<<20));

    pmem2_map_delete(&map);
	pmem2_source_delete(&src);
	pmem2_config_delete(&cfg);
	close(fd);
}