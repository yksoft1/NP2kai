CC=emcc
CXX=em++
AR=emar

ifeq ($(EMSCRIPTEN_ROOT),)
#Set it yourself if EMSCRIPTEN_ROOT not there
	EMSCRIPTEN_ROOT=c:/emsdk/emscripten/1.38.12
endif

#WebAssembly seemed to be a bit faster than asm.js
WASM=1

#Set maximum RAM that Emscripten use (in bytes).
EMSCRIPTEN_TOTAL_MEMORY=67108864

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
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse2/*.c) \
		$(wildcard $(NP2_PATH)/i386c/ia32/instructions/sse3/*.c) \
		$(NP2_PATH)/sdl2/em/main.c

OBJECTS  = $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)
CXXFLAGS += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -DNP2_SDL2  -DUSE_SDLAUDIO -DCPUCORE_IA32 -DSUPPORT_PC9821 -DUSE_FPU -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DSUPPORT_WAB -DSUPPORT_CL_GD5430 -DUSE_MAME -DUSE_MMX -DUSE_SSE -DUSE_SSE2 -DUSE_SSE3 -DSUPPORT_SOUND_SB16 -DSUPPORT_FPU_DOSBOX -DSUPPORT_FPU_DOSBOX2 -DSUPPORT_FPU_SOFTFLOAT -DSUPPORT_GPIB -DUSE_TSC -DSUPPORT_PCI -DSUPPORT_NVL_IMAGES -DUSE_FASTPAGING -DUSE_VME -DBIOS_IO_EMULATION
CFLAGS   += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -DNP2_SDL2  -DUSE_SDLAUDIO -DCPUCORE_IA32 -DSUPPORT_PC9821 -DUSE_FPU -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DSUPPORT_WAB -DSUPPORT_CL_GD5430 -DUSE_MAME -DUSE_MMX -DUSE_SSE -DUSE_SSE2 -DUSE_SSE3 -DSUPPORT_SOUND_SB16 -DSUPPORT_FPU_DOSBOX -DSUPPORT_FPU_DOSBOX2 -DSUPPORT_FPU_SOFTFLOAT -DSUPPORT_GPIB -DUSE_TSC -DSUPPORT_PCI -DSUPPORT_NVL_IMAGES -DUSE_FASTPAGING -DUSE_VME -DBIOS_IO_EMULATION
LDFLAGS  += $(fpic) -lm $(SDL_LIBS) -lSDL   -static

ifeq ($(SUPPORT_NET), 1)
CXXFLAGS += -DSUPPORT_NET -DSUPPORT_LGY98
CFLAGS   += -DSUPPORT_NET -DSUPPORT_LGY98
endif

ifeq ($(EMULARITY), 1)
	CFLAGS	+= -DUSE_EMULARITY_NP2DIR
endif

ifeq ($(TRACE), 1)
	CFLAGS	+= -DTRACE
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
	$(CC) -O3 -s USE_SDL=2 -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST=@d3.txt $(TARGET) \
	--preload-file $(PREFILE) -s BINARYEN_TRAP_MODE="js" -o $(basename $(TARGET)).html 
else
	$(CC) -O3 -s USE_SDL=2 -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST=@d3.txt $(TARGET) -s BINARYEN_TRAP_MODE="js" -o $(basename $(TARGET)).html 
endif
	
%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(basename $(TARGET)).html 
	rm -f $(basename $(TARGET)).js.orig.js
	rm -f $(basename $(TARGET)).js
	rm -f $(basename $(TARGET)).html.mem
	rm -f $(basename $(TARGET)).wasm 	
	rm -f $(basename $(TARGET)).data

install:
	strip $(TARGET)
	cp $(TARGET) /usr/local/bin/


uninstall:
	rm /usr/local/bin/$(TARGET)

