首先说明一下cache的概念：
这里的cache指的是CPU cache，访问速度比内存快，CPU频繁使用的数据可以缓存在这里，用来加快速度。

海思提供的内存申请API中分为两类，一类是不可cache的内存，API是HI_MPI_SYS_MmzAlloc；另一类是可cache的内存，API是HI_MPI_SYS_MmzAlloc_Cached。

关于cache内存海思是这么说明的：

HI_MPI_SYS_MmzAlloc_Cached 与 HI_MPI_SYS_MmzAlloc 接口的区别：通过本接口分配的内存支持 cache缓存，对于频繁使用的内存，最好使用本接口分配内存，这样可以提高 cpu 读写的效率，提升统性能，如用户在使用 ive 算子时，就存在大量数据频繁读写，此时使用此接口来分配内存，就能很好的提高 cpu 的效率。

当 cpu 访问此接口分配的内存时，会将内存中的数据放在 cache 中，而硬件设备 (如 ive)只能访问物理内存，不能访问 cache 的内容，对于这种 cpu 和硬件会共同操作的内存，需调用 HI_MPI_SYS_MmzFlushCache 做好数据同步。

使用cache内存需要注意的一点就是在其他硬件（如GPU）需要使用这部分内存的时候，需要将CPU中缓存的数据先刷新到内存中（使用HI_MPI_SYS_MmzFlushCache），然后再调用相应的接口。

如果做不好同步的话，会遇到各种状况，图像数据不完整等等。