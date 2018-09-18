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

TARGET_NAME := np2kai.bc

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

INCFLAGS += 	-I$(NP2_PATH)/i286c \
		-I$(NP2_PATH)/sdl2/em
SOURCES_C += 	$(wildcard $(NP2_PATH)/i286c/*.c) \
		$(NP2_PATH)/sdl2/em/main.c

OBJECTS  = $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)
CXXFLAGS += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -DNP2_SDL2  -DUSE_SDLAUDIO -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DUSE_MAME -DSUPPORT_SOUND_SB16
CFLAGS   += $(fpic) $(INCFLAGS) $(COMMONFLAGS) -DNP2_SDL2  -DUSE_SDLAUDIO -DSUPPORT_LARGE_HDD -DSUPPORT_VPCVHD -DSUPPORT_KAI_IMAGES -DHOOK_SYSKEY -DALLOW_MULTIRUN -DUSE_MAME -DSUPPORT_SOUND_SB16
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
	$(CC) -O3 -s USE_SDL=2 -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST=@d3.txt $(TARGET) \
	--preload-file $(PREFILE) -o $(basename $(TARGET)).html 
else
	$(CC) -O3 -s USE_SDL=2 -s WASM=$(WASM) -s TOTAL_MEMORY=$(EMSCRIPTEN_TOTAL_MEMORY)  -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 \
	-s EMTERPRETIFY_WHITELIST=@d3.txt $(TARGET) -o $(basename $(TARGET)).html 
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
	rm -f $(basename $(TARGET)).html.mem
	rm -f $(basename $(TARGET)).wasm 	
	rm -f $(basename $(TARGET)).data

install:
	strip $(TARGET)
	cp $(TARGET) /usr/local/bin/


uninstall:
	rm /usr/local/bin/$(TARGET)

