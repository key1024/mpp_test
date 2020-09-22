### 1.mpp介绍
海思提供的媒体处理软件平台(Media Process Platform,简称 MPP)，可支持应用软件快速开发。该平台对应用软件屏蔽了芯片相关的复杂的底层处理，并对应用软件直接提供MPI（MPP Program Interface）接口完成相应功能。该平台支持应用软件快速开发以下功能：输入视频捕获、H.265/H.264/JPEG 编码、H.265/H.264/JPEG 解码、视频输出显示、视频图像前处理（包括去噪、增强、锐化）、图像拼接、图像几何矫正、智能、音频捕获及输出、音频编解码等功能。

### 2.基础环境搭建
之前开发项目，都是把项目放到mpp的目录下进行开始，现在尝试把mpp的头文件和相关库拿出来单独建立项目进行开发。
#### 1.目录结构
```
.
├── bin # 库和可执行文件目录
├── build # 编译目录
│   └── ev300
├── code
│   ├── include
│   │   └── hisi # 海思头文件目录
│   └── src
│       ├── CMakeLists.txt
│       └── main.cpp
└── lib # 静态库文件存放目录
```

其中海思mpp相关的头文件在海思提供的sdk的mpp/include目录下,库文件在mpp/lib。
#### 2.cmake文件：
```c
cmake_minimum_required(VERSION 2.8)

# 生成的项目名
project(mpp_test)

# 设置编译工具
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER "arm-himix100-linux-gcc")
set(CMAKE_CXX_COMPILER "arm-himix100-linux-g++")

# 设置头文件路径
include_directories(../include)
include_directories(../include/hisi)

# 设置链接库路径
link_directories(../../bin)
# 链接库
link_libraries(mpi securec upvqe VoiceEngine dnvqe)
link_libraries(pthread)

#设置项目生成目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../bin)

# 包含源文件
aux_source_directory(./ DIR_SRCS)

# 生成项目
add_executable(mpp_test ${DIR_SRCS})
```

#### 3.测试文件：
```cpp
#include "hi_comm_sys.h"
#include "mpi_sys.h"

int main(void)
{
    // 获取mpp的版本号
    MPP_VERSION_S version;
    HI_S32 ret = HI_MPI_SYS_GetVersion(&version);
    printf("mpp version: %s\n", version.aVersion);

    return 0;
}
```

#### 4.输出
```
mpp version: HI_VERSION=Hi3516EV200_MPP_V1.0.1.0 B050 Release
```

备注：
- 海思芯片型号：3516ev300
- 运行时注意相关驱动是否已经加载