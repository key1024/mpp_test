# 包含源文件
aux_source_directory(./ DIR_SRCS)
aux_source_directory(../common/ DIR_SRCS)
aux_source_directory(../hisi_common/ DIR_SRCS)

# 生成项目
add_executable(play_mipi_camera ${DIR_SRCS})
