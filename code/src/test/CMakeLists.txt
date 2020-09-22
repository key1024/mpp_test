# 包含源文件
aux_source_directory(./ DIR_SRCS)

# 生成项目
add_executable(test ${DIR_SRCS})