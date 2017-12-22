
#include "sal_himm.h"
#include "sal_debug.h"

#define PAGE_SIZE 0x1000
#define PAGE_SIZE_MASK 0xfffff000
#define DEFAULT_MD_LEN 128

static void* memmap(unsigned int phy_addr, unsigned int size)
{
    unsigned int phy_addr_in_page = 0;
    unsigned int page_diff = 0;
    unsigned int size_in_page = 0;
    char *addr=NULL;

    /* dev not opened yet, so open it */
    static int fd = -1;
    if (fd < 0)
    {
        fd = open ("/dev/mem", O_RDWR | O_SYNC);
        CHECK(fd > 0, NULL, "failed to open[%s] with: %s\n", "/dev/mem", strerror(errno));
    }

    /* addr align in page_size(4K) */
    phy_addr_in_page = phy_addr & PAGE_SIZE_MASK;
    page_diff = phy_addr - phy_addr_in_page;

    /* size in page_size */
    size_in_page =((size + page_diff - 1) & PAGE_SIZE_MASK) + PAGE_SIZE;
    addr = (char *)mmap ((char *)0, size_in_page, PROT_READ|PROT_WRITE, MAP_SHARED, fd, phy_addr_in_page);
    CHECK(addr != MAP_FAILED, NULL, "Error with: %s\n", strerror(errno));

    return (char *)(addr+page_diff);
}

static int unmemmap(char* addr, unsigned int phy_addr, unsigned int size)
{
    int ret = -1;
    unsigned int phy_addr_in_page = 0;
    unsigned int page_diff = 0;
    unsigned int size_in_page = 0;

    /* addr align in page_size(4K) */
    phy_addr_in_page = phy_addr & PAGE_SIZE_MASK;
    page_diff = phy_addr - phy_addr_in_page;

    /* size in page_size */
    size_in_page =((size + page_diff - 1) & PAGE_SIZE_MASK) + PAGE_SIZE;
    ret = munmap(addr-page_diff, size_in_page);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    return 0;
}

unsigned int himm_write(unsigned int addr, unsigned int data)
{
    int ret = -1;
    //unsigned int ulOld;
    unsigned int ulNew = data;
    void* pMem  = NULL;

    pMem = memmap(addr, DEFAULT_MD_LEN);
    CHECK(pMem, -1, "Error with: %#x\n", pMem);
    //ulOld = *(unsigned int*)pMem;
    //printf("pMem: %p, addr: %#x, 0x%08x --> 0x%08x \n\n\n\n\n", pMem, addr, ulOld, ulNew);
    *(unsigned int*)pMem = ulNew;

    ret = unmemmap(pMem, addr, DEFAULT_MD_LEN);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    return 0;
}

unsigned int himm_read(unsigned int addr)
{
    int ret = -1;
    unsigned int ulOld = 0;
    //unsigned int ulNew = data;
    void* pMem  = NULL;

    pMem = memmap(addr, DEFAULT_MD_LEN);
    CHECK(pMem, -1, "Error with: %#x\n", pMem);

    ulOld = *(unsigned int*)pMem;
    //printf("old: 0x%08lX\n", ulOld);

    ret = unmemmap(pMem, addr, DEFAULT_MD_LEN);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    return ulOld;
}




