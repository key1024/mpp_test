### 1.说明
海思芯片的物理内存被划分为两个部分，一块供OS使用，另一块就是MMZ(Media Memory Zone,多媒体内存区域)。操作MMZ内存需要HI_MPI提供的接口。malloc不会申请到MMZ里的内存。
MMZ内存的使用情况可以通过`cat /proc/media-mem`命令查看
```
~ # cat /proc/media-mem 
+---ZONE: PHYS(0x44000000, 0x47FFFFFF), GFP=0, nBYTES=65536KB,    NAME="anonymous"
   |-MMB: phys(0x44000000, 0x44007FFF), kvirt=0xC4B98000, flags=0x00000001, length=32KB,    name="sys_scale_coef"
   |-MMB: phys(0x44008000, 0x4405AFFF), kvirt=0xC4D00000, flags=0x00000001, length=332KB,    name="VGS_NodeBuf"
   |-MMB: phys(0x4405B000, 0x441EFFFF), kvirt=0x00000000, flags=0x00000000, length=1620KB,    name="hifb_layer0"
   |-MMB: phys(0x441F0000, 0x44224FFF), kvirt=0xC5600000, flags=0x00000001, length=212KB,    name="IVE_QUEUE"
   |-MMB: phys(0x44225000, 0x44225FFF), kvirt=0xC4ABA000, flags=0x00000003, length=4KB,    name="IVE_TEMP_NODE"

---MMZ_USE_INFO:
 total size=65536KB(64MB),used=2200KB(2MB + 152KB),remain=63336KB(61MB + 872KB),zone_number=1,block_number=5
```
从上图我们可以看出，系统只分配了一块MMZ，名字是'anonymous'，这块MMZ下面目前申请了五块内存。
系统内存使用情况可以通过`free`命令查看
```
/ # free -m
             total       used       free     shared    buffers     cached
Mem:            57         18         38          0          0          7
-/+ buffers/cache:         11         45
Swap:            0          0          0
```
从上面能看出来，系统占有57M的内存，MMZ占有64M的内存。

### 2.示例代码
```c
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
```
```
运行结果：
malloc: 0x1f2d058
phy: 0x44226000
vir: 0xb6b31000
phy_cache: 0x44326000
vir_cache: 0xb6a31000
phy_addr: 0x44326000 cache: 1
```
```
~ # cat /proc/media-mem 
+---ZONE: PHYS(0x44000000, 0x47FFFFFF), GFP=0, nBYTES=65536KB,    NAME="anonymous"
   |-MMB: phys(0x44000000, 0x44007FFF), kvirt=0xC4B98000, flags=0x00000001, length=32KB,    name="sys_scale_coef"
   |-MMB: phys(0x44008000, 0x4405AFFF), kvirt=0xC4D00000, flags=0x00000001, length=332KB,    name="VGS_NodeBuf"
   |-MMB: phys(0x4405B000, 0x441EFFFF), kvirt=0x00000000, flags=0x00000000, length=1620KB,    name="hifb_layer0"
   |-MMB: phys(0x441F0000, 0x44224FFF), kvirt=0xC5600000, flags=0x00000001, length=212KB,    name="IVE_QUEUE"
   |-MMB: phys(0x44225000, 0x44225FFF), kvirt=0xC4ABA000, flags=0x00000003, length=4KB,    name="IVE_TEMP_NODE"
   |-MMB: phys(0x44226000, 0x44325FFF), kvirt=0x00000000, flags=0x00000000, length=1024KB,    name="test"
   |-MMB: phys(0x44326000, 0x44425FFF), kvirt=0x00000000, flags=0x00000000, length=1024KB,    name="test_cache"

---MMZ_USE_INFO:
 total size=65536KB(64MB),used=4248KB(4MB + 152KB),remain=61288KB(59MB + 872KB),zone_number=1,block_number=7

```
可以看到media-mem里面有两块我们申请的内存。
从上面也能看出来MMZ的起始地址是0x44000000，malloc申请的内存地址是0x1f2d058，可以说明系统内存和mmz内存是两块内存。

### 3.关于cache属性的说明
首先说明一下cache的概念：

这里的cache指的是CPU cache，访问速度比内存快，CPU频繁使用的数据可以缓存在这里，用来加快速度。

海思提供的内存申请API中分为两类，一类是不可cache的内存，API是HI_MPI_SYS_MmzAlloc；另一类是可cache的内存，API是HI_MPI_SYS_MmzAlloc_Cached。

关于cache内存海思是这么说明的：

HI_MPI_SYS_MmzAlloc_Cached 与 HI_MPI_SYS_MmzAlloc 接口的区别：通过本接口分配的内存支持 cache缓存，对于频繁使用的内存，最好使用本接口分配内存，这样可以提高 cpu 读写的效率，提升统性能，如用户在使用 ive 算子时，就存在大量数据频繁读写，此时使用此接口来分配内存，就能很好的提高 cpu 的效率。

当 cpu 访问此接口分配的内存时，会将内存中的数据放在 cache 中，而**硬件设备 (如 ive)只能访问物理内存，不能访问 cache 的内容**，对于这种 cpu 和硬件会共同操作的内存，需调用 HI_MPI_SYS_MmzFlushCache 做好数据同步。

使用cache内存需要注意的一点就是在其他硬件（如GPU）需要使用这部分内存的时候，需要将CPU中缓存的数据先刷新到内存中（使用HI_MPI_SYS_MmzFlushCache），然后再调用相应的接口。

如果做不好同步的话，会遇到各种状况，图像数据不完整等等。