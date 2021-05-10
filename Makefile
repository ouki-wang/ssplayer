CROSS_COMPILE ?=arm-linux-gnueabihf-
CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

ENABLE_HDMI = 0
USE_FFMPEG  = 1

COM_FLAGS = -Wall -O2 -fPIC -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=hard -mthumb-interwork -marm
C_FLAGS	   = $(COM_FLAGS) -std=c11
CPP_FLAGS  = $(COM_FLAGS) -std=c++11

INCLUDES  += -I. -I./sstar/include -I./ffmpeg-4.1.3/include/

TARGET_NAME  = SsPlayer

CPP_SRCS  =  

C_SRCS    =  SsPlayeEs.c sstardisp.c
			
CPP_OBJS = $(patsubst %.cpp, %.cpp.o, $(CPP_SRCS))
C_OBJS = $(patsubst %.c, %.c.o, $(C_SRCS))

LIB_PATH  = -L. -L./sstar/lib -L./ffmpeg-4.1.3/lib

ifeq ($(USE_FFMPEG), 1)
LIB_NAME  = -lavformat -lavcodec -lavutil
endif

LIB_NAME += -lm -lmi_vdec -lmi_sys -lmi_disp -lmi_ao -lmi_common -ldl

ifeq ($(ENABLE_HDMI), 1)
LIB_PATH += -lmi_hdmi
else
LIB_NAME += -lmi_panel
endif


.PHONY: all prepare clean

all: prepare $(TARGET_NAME) finish

prepare:
	@echo
	@echo ">>>>========================================================"
	@echo "TARGET_NAME = $(TARGET_NAME)"
	@echo


clean:
	@rm -Rf $(CPP_OBJS)
	@rm -f $(C_OBJS)
	@rm -Rf $(TARGET_NAME)

finish:
	@echo "make done"
	@echo "<<<<========================================================"
	@echo

$(TARGET_NAME): $(CPP_OBJS) $(CPP_SRCS) $(C_OBJS) $(C_SRCS)
	@echo "generate $@"
	@$(CC) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIB_PATH) $(LIB_NAME) -lm -lpthread

%.c.o : %.c
	@echo "compile $@"
	@$(CC) -DENABLE_HDMI=$(ENABLE_HDMI) -DUSE_FFMPEG=$(USE_FFMPEG) $(C_FLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) -DENABLE_HDMI=$(ENABLE_HDMI) -DUSE_FFMPEG=$(USE_FFMPEG) $(CPP_FLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@
