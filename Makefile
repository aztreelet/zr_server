# V1.0 20230622
# poorwebserver极其简单的webserver实现
# BUG: 如果vpath不包含cgi（cgi目录和.c.h文件都存在），编译时还是需要cgi
VERSION := 1.0

# 模块目标文件
TARGET_SER := webserver
# 总的编译目标
TARGET_ALL := $(TARGET_SER)

# 编译器选项
CC := gcc
DEBUG_FLAG := -g
CFLAGS := -Wall
# 链接库
LIBS := -lpthread

# 编译生成文件存放目录
BUILD := ./build

# 查找当前路径下所有的.c源文件，并获取不包括路径的文件名称
SRCS := $(shell find ./ -name "*.c")
SRCS_WITHOUT_DIR := $(notdir $(SRCS))

# OBJS：对于所有.c都要编译出对应的.c
OBJS := $(SRCS_WITHOUT_DIR:.c=.o)
# 添加上前缀路径
OBJS := $(addprefix $(BUILD)/, $(OBJS))

# 创建要生成的.d文件列表（每一个.c也要有一个对应的.d文件，内容是该.c->.o时所依赖的所有文件
# gcc -MM 生成（注：-M会包括库文件，-MM不会
DEPDIR := .deps
DEPFILES := $(SRCS_WITHOUT_DIR:%.c=$(DEPDIR)/%.d)
# DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

vpath %.c util
vpath %.c http
vpath %.c cgi
vpath %.c server
vpath %.c io
vpath %.c thread_pool
# vpath %.c signal
# vpath %.c timer

# 仅编译生成all指定的程序
all: $(TARGET_ALL)

# 将所有.d依赖文件中的内容包含进来
include $(DEPFILES)

$(TARGET_ALL) : $(OBJS)
		$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(OBJS): $(BUILD)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) -c $(DEBUG_FLAG) $(CFLAGS) $< -o $@ $(LIBS)

# 生成.d文件
$(DEPFILES) : $(DEPDIR)/%.d : %.c | $(DEPDIR)
	@echo [MAKE INFO] create the .d file firstly
	$(CC) -MM $< > $@ 

$(DEPDIR):
	@echo [MAKE INFO] create the depdir
	@mkdir -p $(DEPDIR)


.PHONY: clean debug

clean :
	rm -rf $(TARGET_ALL) .deps $(OBJS)

# 新建工程时使用debug测试各目标文件是否正确
debug :
	@echo OBJS is[$(OBJS)]
	@echo DEPFILES is[$(DEPFILES)]
	@echo SRCS_WITHOUT_DOR is[$(SRCS_WITHOUT_DIR)]
	@echo ALL_VPATH is[$(SRCS_WITHOUT_DIR)]

