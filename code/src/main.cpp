#include <stdio.h>
#include <unistd.h>
#include "hi_comm_sys.h"
#include "mpi_sys.h"

// 时间戳相关
void pts()
{
    // 设置时间戳基准，系统时间不会变
    HI_S32 ret = HI_MPI_SYS_InitPTSBase(0);
    if(ret != HI_SUCCESS)
        printf("init pts base failed\n");

    HI_U64 pts = 0;
    ret = HI_MPI_SYS_GetCurPTS(&pts);
    if(ret != HI_SUCCESS)
        printf("get current pts failed\n");
    else
        printf("pts: %llu\n", pts);
        
    sleep(1);

    ret = HI_MPI_SYS_SyncPTS(0);
    if(ret != HI_SUCCESS)
        printf("sync pts failed\n");
    ret = HI_MPI_SYS_GetCurPTS(&pts);
    if(ret != HI_SUCCESS)
        printf("get current pts failed\n");
    else
        printf("pts: %llu\n", pts);  
}

void mmap_test()
{
    // 使用系统接口申请内存
    void *data = malloc(1024);
    printf("malloc: %#x\n", data);

    // 申请MMZ内存
    HI_U64 phy_addr = 0;
    HI_VOID *vir_addr = NULL;
    HI_MPI_SYS_MmzAlloc(&phy_addr, &vir_addr, "test", "anonymous", 1024*1024);
    printf("phy: %#llx\n", phy_addr);
    printf("vir: %#x\n", vir_addr);

    // 申请支持cache的内存
    HI_U64 phy_addr_cache = 0;
    HI_VOID *vir_addr_cache = NULL;
    HI_MPI_SYS_MmzAlloc_Cached(&phy_addr_cache, &vir_addr_cache, "test_cache", "anonymous", 1024*1024);
    printf("phy_cache: %#llx\n", phy_addr_cache);
    printf("vir_cache: %#x\n", vir_addr_cache);

    // 可以根据虚拟内存地址获取内存信息，使用malloc申请的内存无法使用这个函数
    SYS_VIRMEM_INFO_S mem_info;
    HI_S32 ret = HI_MPI_SYS_GetVirMemInfo(vir_addr_cache, &mem_info);
    if(ret != HI_SUCCESS)
        printf("get vir mem info failed.\n");
    else
        printf("phy_addr: %#llx cache: %d\n", mem_info.u64PhyAddr, mem_info.bCached);

    getchar();

    HI_MPI_SYS_MmzFree(phy_addr, NULL);
    HI_MPI_SYS_MmzFree(phy_addr_cache, NULL);
    free(data);
}

int main(void)
{
    MPP_VERSION_S version;
    HI_S32 ret = HI_MPI_SYS_GetVersion(&version);
    printf("mpp version: %s\n", version.aVersion);

    mmap_test();

    return 0;
}