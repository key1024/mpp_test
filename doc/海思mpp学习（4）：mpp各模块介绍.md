### 1.简单介绍
mpp根据功能划分为很多模块，有：视频输入（VI）、视频处理（VPSS）、视频编码（VENC）、视频解码（VDEC）、视频输出(VO)、视频拼接(AVS)、音频输入(AI)、音频输出(AO)、音频编码（AENC）音频解码（ADEC）、区域管理（REGION）等模块。其中我们频繁会用到的模块有VI，VPSS，VENC，VDEC这几个模块。

关于每个模块的具体功能和使用，可以通过后面的例子边学习边了解，刚开始先简单了解即可。

### 2.系统绑定
MPP 提供系统绑定接口（HI_MPI_SYS_Bind），即通过数据接收者绑定数据源来建立两者之间的关联关系（只允许数据接收者绑定数据源）。绑定后，数据源生成的数据将自动发送给接收者。

### 3.VI和VPSS工作模式
VI 和 VPSS 各自的工作模式分为在线，离线，并行模式

模式说明：
|模式|VI_CAP 与 VI_PROC|VI_PROC 与 VPSS|
|:--:|:--:|:--:|
|在线模式|VI_CAP 与 VI_PROC 之间在线数据流传输，此模式下 VI_CAP不会写出 RAW 数据到 DDR，而是直接把数据流送给VI_PROC。|VI_PROC 与 VPSS 之间的在线数据流传输，在此模式下VI_PROC不会写出 YUV 数据到 DDR，而是直接把数据流送给 VPSS。|
|离线模式|VI_CAP 写出 RAW 数据到DDR，然后 VI_PROC 从 DDR读取 RAW 数据进行后处理。|VI_PROC 写出 YUV 数据到DDR，然后 VPSS 从 DDR 读取YUV 数据进行后处理。|
|并行模式|当对接大数据量的时序，例如8K@30fps 时，需要 VI_CAP 与两个 VI_PROC 处于并行模式，VI_CAP 直接把一帧数据送给两个 VI_PROC 并行处理。|当对接大数据量的时序，例如8K@30fps 时，需要 VI_CAP 与两个 VI_PROC 处于并行模式，同时两个 VPSS 也分别与VI_PROC 处于并行模式，VI_CAP 直接把一帧数据送给两个 VI_PROC 并行处理，再给VPSS 并行处理。|

TODO 补充细节