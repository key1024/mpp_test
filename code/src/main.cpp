#include <stdio.h>
#include <unistd.h>
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "hi_buffer.h"

// 时间戳相关
void pts()
{
    HI_U64 pts = 0;
    HI_S32 ret = HI_MPI_SYS_GetCurPTS(&pts);
    if(ret != HI_SUCCESS)
        printf("get current pts failed\n");
    else
        printf("pts: %llu\n", pts);

    // 设置时间戳基准，系统时间不会变
    ret = HI_MPI_SYS_InitPTSBase(0);
    if(ret != HI_SUCCESS)
        printf("init pts base failed\n");
    else
        printf("init pts success\n");

    sleep(1);

    // ret = HI_MPI_SYS_SyncPTS(0);
    // if(ret != HI_SUCCESS)
    //     printf("sync pts failed\n");
    ret = HI_MPI_SYS_GetCurPTS(&pts);
    if(ret != HI_SUCCESS)
        printf("get current pts failed\n");
    else
        printf("pts: %llu\n", pts);  
}

// 内存映射
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

    // mmap
    HI_VOID* tmp = HI_MPI_SYS_Mmap(phy_addr, 1024*1024);
    if(!tmp)
        printf("HI_MPI_SYS_Mmap falied\n");
    else
        printf("tmp addr: %#x\n", tmp);

    HI_VOID* tmp_cache = HI_MPI_SYS_Mmap(phy_addr_cache, 1024*1024);
    if(!tmp)
        printf("HI_MPI_SYS_Mmap cache mem falied\n");
    else
        printf("tmp cache addr: %#x\n", tmp_cache);

    tmp = HI_MPI_SYS_MmapCache(phy_addr, 1024*1024);
    if(!tmp)
        printf("HI_MPI_SYS_MmapCache falied\n");
    else
        printf("tmp addr: %#x\n", tmp);
    tmp_cache = HI_MPI_SYS_MmapCache(phy_addr_cache, 1024*1024);
    if(!tmp)
        printf("HI_MPI_SYS_MmapCache cache mem falied\n");
    else
        printf("tmp cache addr: %#x\n", tmp_cache);

    // 测试1.HI_MPI_SYS_MflushCache和HI_MPI_SYS_MmzFlushCache的区别
    ret = HI_MPI_SYS_MflushCache(phy_addr_cache, vir_addr_cache, 1024*1024);
    if(ret != HI_SUCCESS)
        printf("HI_MPI_SYS_MflushCache failed\n");
    else
        printf("HI_MPI_SYS_MflushCache success\n");

    ret = HI_MPI_SYS_MmzFlushCache(phy_addr_cache, vir_addr_cache, 1024*1024);
    if(ret != HI_SUCCESS)
        printf("HI_MPI_SYS_MmzFlushCache failed\n");
    else
        printf("HI_MPI_SYS_MmzFlushCache success.\n");

    getchar();

    HI_MPI_SYS_MmzFree(phy_addr, NULL);
    HI_MPI_SYS_MmzFree(phy_addr_cache, NULL);
    free(data);
}

void vb_test()
{
    VB_CONFIG_S config;
    memset(&config, 0, sizeof(VB_CONFIG_S));
    config.u32MaxPoolCnt = 1;
    HI_U64 blk_size = COMMON_GetPicBufferSize(1920,1080, PIXEL_FORMAT_YVU_SEMIPLANAR_420, 
                                            DATA_BITWIDTH_8,COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    config.astCommPool[0].u64BlkSize = blk_size;
    printf("blk size: %llu\n", config.astCommPool[0].u64BlkSize);
    config.astCommPool[0].u32BlkCnt = 3;

    // 设置公共视频缓存池属性
    HI_S32 ret = HI_MPI_VB_SetConfig(&config);
    if(ret != HI_SUCCESS)
    {
        printf("HI_MPI_VB_SetConfig falied: %#x\n", ret);
        return;
    }

    // 初始化缓存池
    ret = HI_MPI_VB_Init();
    if(ret != HI_SUCCESS)
    {
        printf("HI_MPI_VB_Init falied: %#x\n", ret);
        return;
    }

    // mpp系统初始化
    ret = HI_MPI_SYS_Init();
    if(ret != HI_SUCCESS)
    {
        printf("HI_MPI_SYS_Init falied: %#x\n", ret);
        return;
    }

    // 从公共视频缓存池获取一个缓存块
    VB_BLK blk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, blk_size, NULL);
    if(blk == VB_INVALID_HANDLE)
        printf("HI_MPI_VB_GetBlock failed\n");
    else
        printf("blk id: %u\n", blk);

    HI_U64 phy_addr = HI_MPI_VB_Handle2PhysAddr(blk);
    if(phy_addr == 0)
        printf("HI_MPI_VB_Handle2PhysAddr failed\n");
    else
        printf("phy addr: %llx\n", phy_addr);

    // 通过mmap获取虚拟地址
    HI_VOID* tmp_addr = HI_MPI_SYS_Mmap(phy_addr, blk_size);
    if(!tmp_addr)
        printf("HI_MPI_SYS_Mmap falied\n");
    else
        printf("tmp addr: %#x\n", tmp_addr);

    // 使用VB提供的接口获取虚拟地址
    // 需要先把缓存池映射出虚拟地址，才能获取其中缓存块的虚拟地址
    ret = HI_MPI_VB_MmapPool(0);
    if(ret != HI_SUCCESS)
        printf("HI_MPI_VB_MmapPool falied: %#x\n", ret);

    HI_VOID *vir_addr = NULL;
    ret = HI_MPI_VB_GetBlockVirAddr(0, phy_addr, &vir_addr);
    if(ret != HI_SUCCESS)
        printf("HI_MPI_VB_GetBlockVirAddr failed %#x\n", ret);
    else
        printf("vir addr: %#x\n", vir_addr);

    // tmp_addr和vir_addr指向同一块内存
    char str[] = "hello";
    memcpy(tmp_addr, str, strlen(str));
    printf(">>>%s<<<\n", (char*)vir_addr);
    
    getchar();

    HI_MPI_SYS_Munmap(tmp_addr, blk_size);

    HI_MPI_VB_ReleaseBlock(blk);

    // 需要保证对应的缓存池中没有被MPI层占用
    HI_MPI_VB_MunmapPool(0);

    ret = HI_MPI_SYS_Exit();
    if(ret != HI_SUCCESS)
    {
        printf("HI_MPI_SYS_Exit falied: %#x\n", ret);
        return;
    }    

    ret = HI_MPI_VB_Exit();
    if(ret != HI_SUCCESS)
    {
        printf("HI_MPI_VB_Exit falied: %#x\n", ret);
        return;
    }
}

int main(void)
{
    MPP_VERSION_S version;
    HI_S32 ret = HI_MPI_SYS_GetVersion(&version);
    printf("mpp version: %s\n", version.aVersion);

    // pts();

    // mmap_test();

    vb_test();

    return 0;
}