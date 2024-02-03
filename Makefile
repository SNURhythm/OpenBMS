CXX?=g++
CXXFLAGS?=-std=c++17 -Wall -pedantic -Werror -Wshadow -Wstrict-aliasing -Wstrict-overflow
BUILD_DIR=build
LIB_DIR=lib
SRC_DIR=src
ifeq ($(OS),Windows_NT)
    CP_RES = xcopy /s /i res $(BUILD_DIR)\\res /Y
	CP = copy
	RM = del
	RRM = rmdir /s /q	
	MKDIRP = mkdir
	IGNORE_ERRORS = 2>NUL || (exit 0)
	# Attempt to detect a Unix-like shell environment (e.g., Git Bash, Cygwin) by checking for /bin/sh
    ifneq ($(shell if [ -x /bin/sh ]; then echo true; fi),)
        CP = cp
        RM = rm -f
        RRM = rm -rf
        MKDIRP = mkdir -p
		IGNORE_ERRORS = 2>/dev/null || true
    endif

    CCFLAGS += -D WIN32
		SDL2FLAGS=-lmingw32 -lSDL2main -lSDL2 -Iinclude -mwindows
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CCFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CCFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CCFLAGS += -D IA32
        endif
    endif
else
	CP_RES = cp -r res $(BUILD_DIR)
	CP = cp
	RM = rm
	RRM = rm -rf
	MKDIRP = mkdir -p
	IGNORE_ERRORS = 2>/dev/null || true
  	SDL2FLAGS=$(shell pkg-config sdl2 --cflags --libs)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CCFLAGS += -D LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        CCFLAGS += -D OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CCFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CCFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CCFLAGS += -D ARM
    endif
endif


.PHONY: all msg clean fullclean

all: msg main

$(BUILD_DIR):
	@$(MKDIRP) $(BUILD_DIR)


msg:
	@echo '--- C++11 ---'

main: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	${CXX} ${CXXFLAGS} -O2 -o $(BUILD_DIR)/$@ $< ${SDL2FLAGS}
	$(CP_RES)

windows: main
	$(CP) $(LIB_DIR)/win32/*.dll $(BUILD_DIR)

darwin: main
	$(CP) $(LIB_DIR)/darwin/*.dylib $(BUILD_DIR)

linux: main
	$(CP) $(LIB_DIR)/linux/*.so $(BUILD_DIR)

small: main.cpp
	${CXX} ${CXXFLAGS} -Os -o main $< ${SDL2FLAGS}
	-strip main
	-sstrip main

debug: main.cpp
	${CXX} ${CXXFLAGS} -O0 -g -o main $< ${SDL2FLAGS}

asm: main.asm

main.asm: main.cpp
	${CXX} ${CFLAGS} -S -masm=intel -Og -o main.asm $< ${SDL2FLAGS}

run: msg main
	time ./main

clean:
	$(RM) main *.o main.asm $(IGNORE_ERRORS)
	$(RRM) $(BUILD_DIR) $(IGNORE_ERRORS)

fullclean: clean
