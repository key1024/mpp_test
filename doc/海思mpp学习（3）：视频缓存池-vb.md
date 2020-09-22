### 1.说明
视频缓存池主要向媒体业务提供大块物理内存管理功能，必须在系统初始化之前配置公共视频缓存池。
### 2.示例代码
```c
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
    // 公共缓存池的pool id是0
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
    memset(tmp_addr, 0, blk_size);
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
```
运行结果：
```
blk size: 3110400
blk id: 0
phy addr: 441f0000
tmp addr: 0xb6990000
vir addr: 0xb60a9000
>>>hello<<<
```
从运行结果可以看到，HI_MPI_SYS_Mmap可以直接映射出缓存块的虚拟地址，HI_MPI_VB_GetBlockVirAddr需要先映射出整个缓存池的虚拟地址，才能映射出自己的虚拟地址，HI_MPI_SYS_Mmap这个函数更常用一些。

程序运行过程中可以使用`cat /proc/media-mem`和`cat /proc/umap/vb`分别查看mmz内存和vb的使用情况。
```
~ # cat /proc/media-mem 
+---ZONE: PHYS(0x44000000, 0x47FFFFFF), GFP=0, nBYTES=65536KB,    NAME="anonymous"
   |-MMB: phys(0x44000000, 0x44007FFF), kvirt=0xC4B98000, flags=0x00000001, length=32KB,    name="sys_scale_coef"
   |-MMB: phys(0x44008000, 0x44008FFF), kvirt=0xC56BB000, flags=0x00000001, length=4KB,    name="VbPool_supNC"
   |-MMB: phys(0x44009000, 0x4403DFFF), kvirt=0xC5A80000, flags=0x00000001, length=212KB,    name="IVE_QUEUE"
   |-MMB: phys(0x4403E000, 0x4403EFFF), kvirt=0xC58E4000, flags=0x00000003, length=4KB,    name="IVE_TEMP_NODE"
   |-MMB: phys(0x4405B000, 0x441EFFFF), kvirt=0x00000000, flags=0x00000000, length=1620KB,    name="hifb_layer0"
   |-MMB: phys(0x441F0000, 0x44AD6FFF), kvirt=0x00000000, flags=0x00000000, length=9116KB,    name="VbPool"
   |-MMB: phys(0x44AD7000, 0x44B29FFF), kvirt=0xC5A00000, flags=0x00000001, length=332KB,    name="VGS_NodeBuf"

---MMZ_USE_INFO:
 total size=65536KB(64MB),used=11320KB(11MB + 56KB),remain=54216KB(52MB + 968KB),zone_number=1,block_number=7
```
可以看出，视频缓存池-VbPool是在mmz内存中创建的。
```
~ # cat /proc/umap/vb 

[VB] Version: [Hi3516EV200_MPP_V1.0.1.0 B050 Release], Build Time[May  9 2019, 22:51:50]

-----VB PUB CONFIG--------------------------------------------------------------
MaxPoolCnt
       512
-----VB SUPPLEMENT ATTR---------------------------------------------------------
  Config    Size   VbCnt
       0       0       3
-----COMMON POOL CONFIG---------------------------------------------------------
PoolId         0         1         2         3         4         5         6         7         8         9        10        11        12        13        14        15
Size     3110400         0         0         0         0         0         0         0         0         0         0         0         0         0         0         0
Count          3         0         0         0         0         0         0         0         0         0         0         0         0         0         0         0

--------------------------------------------------------------------------------
PoolId  PhysAddr            VirtAddr            IsComm  Owner  BlkSz     BlkCnt  Free    MinFree 
0       0x441f0000          0x0                 1       -1     3110400   3       2       2       
BLK   VI    VO    VGS   VENC  VDEC  H265E H264E JPEGE H264D JPEGD VPSS  DIS   USER  PCIV  AI    AENC  RC    VFMW  GDC   AVS   RECT  MATCH MCF   
0     0     0     0     0     0     0     0     0     0     0     0     0     1     0     0     0     0     0     0     0     0     0     0     
Sum   0     0     0     0     0     0     0     0     0     0     0     0     1     0     0     0     0     0     0     0     0     0     0     
```