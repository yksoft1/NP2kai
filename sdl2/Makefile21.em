CC=emcc
CXX=em++
AR=emar

ifeq ($(EMSCRIPTEN_ROOT),)
#Set it yourself if EMSCRIPTEN_ROOT not there
	EMSCRIPTEN_ROOT=c:/emsdk/emscripten/1.38.12
endif

DEBUG=0
SUPPORT_NET?= 0
#STATIC_LINKING=1

SDL_CONFIG ?= $(EMSCRIPTEN_ROOT)/system/bin/sdl2-config
SDL_CFLAGS := $(shell $(SDL_CONFIG) --cflags) -DUSE_SDL_CONFIG
SDL_LIBS := $(shell $(SDL_CONFIG) --libs)

TARGET_NAME := np21kai.bc

TARGET := $(TARGET_NAME)
LDFLAGS :=
fpic = -fPIC

ifeq ($(DEBUG), 1)
COMMONFLAGS += -O0 -g
else
COMMONFLAGS += -O3 -DNDEBUG -DGIT_VERSION=\"0\"
endif

CORE_DIR    := ..
SOURCES_C   :=
SOURCES_CXX :=

include Makefile.common

INCFLAGS := $(SDL_CFLAGS) $(INCFLAGS)

INCFLAGS += 	-I$(NP2_PATH)/i386c \
		-I$(NP2_PATH)/i386c/ia32 \
		-I$(NP2_PATH)/i386c/ia32/instructions \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu \
		-I$(NP2_PATH)/i386c/ia32/instructions/fpu/softfloat \
		-I$(NP2_PATH)/i386c/ia32/instructions/mmx \
		-I$(NP2_PATH)/i386c/ia32/instructions/sse \
		-I$(NP2_PATH)/sdl2/em
SOURCES_C += 	$(wildcard $(NP2_PATH)/i386c/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/*.c) \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpdummy.c \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpemul_dosbox.c \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpemul_dosbox2.c \
		$(NP2_PATH)/i386c/ia32/instructions/fpu/fpemul_softfloat.c \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/fpu/softfloat/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/mmx/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse/*.c) \
		$(NP2_PATH)/sdl2/em/main.c

OBJECTS  = $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)
CXXFLAGS += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -s WASM=1 -s TOTAL_MEMORY=134217728 -DNP2_SDL2  -DUSE_SDLAUDIO -DCPUCORE_IA32 -DSUPPORT_PC9821 -DUSE_FPU -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DSUPPORT_WAB -DSUPPORT_CL_GD5430 -DUSE_MAME -DSUPPORT_SOUND_SB16 -DSUPPORT_FPU_DOSBOX -DSUPPORT_FPU_DOSBOX2 -DSUPPORT_FPU_SOFTFLOAT -DSUPPORT_GPIB  -DUSE_TSC
CFLAGS   += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -s WASM=1 -s TOTAL_MEMORY=134217728 -DNP2_SDL2  -DUSE_SDLAUDIO -DCPUCORE_IA32 -DSUPPORT_PC9821 -DUSE_FPU -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DSUPPORT_WAB -DSUPPORT_CL_GD5430 -DUSE_MAME -DSUPPORT_SOUND_SB16 -DSUPPORT_FPU_DOSBOX -DSUPPORT_FPU_DOSBOX2 -DSUPPORT_FPU_SOFTFLOAT -DSUPPORT_GPIB  -DUSE_TSC
LDFLAGS  += $(fpic) -lm $(SDL_LIBS) -lSDL   -static

ifeq ($(SUPPORT_NET), 1)
CXXFLAGS += -DSUPPORT_NET -DSUPPORT_LGY98
CFLAGS   += -DSUPPORT_NET -DSUPPORT_LGY98
endif


all: $(TARGET)
$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
endif

html: $(TARGET)
ifeq ($(PRELOAD), 1)
	$(CXX) -O3 -s USE_SDL=2 -s TOTAL_MEMORY=134217728  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST=@d3.txt --profiling-funcs  $(TARGET) \
	--preload-file $(PREFILE) -o $(basename $(TARGET)).html 
else
	$(CXX) -O3 -s USE_SDL=2 -s TOTAL_MEMORY=134217728  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST=@d3.txt --profiling-funcs  $(TARGET) -o $(basename $(TARGET)).html 
endif
	
%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(basename $(TARGET)).html 
	rm -f $(basename $(TARGET)).orig.js
	rm -f $(basename $(TARGET)).js
	rm -f $(basename $(TARGET)).wasm 	

install:
	strip $(TARGET)
	cp $(TARGET) /usr/local/bin/


uninstall:
	rm /usr/local/bin/$(TARGET)

