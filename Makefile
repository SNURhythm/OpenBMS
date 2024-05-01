CXX=g++
CXXFLAGS?=-std=c++17 -Wall -pedantic -Werror -Wno-ignored-attributes -Wstrict-aliasing -Wstrict-overflow -Iinclude 
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
	VLC_PLUGINS_TAR=libvlcplugins\\win32.tar.gz
	SLASH=\\
	# Attempt to detect a Unix-like shell environment (e.g., Git Bash, Cygwin) by checking for /bin/sh
    ifneq ($(shell if [ -x /bin/sh ]; then echo true; fi),)
        CP = cp
        RM = rm -f
        RRM = rm -rf
        MKDIRP = mkdir -p
		IGNORE_ERRORS = 2>/dev/null || true
		SLASH=/
    endif

    CXXFLAGS += -D WIN32
		SDL2FLAGS=-lmingw32 -lSDL2main -lSDL2# -mwindows
		VLCFLAGS=-L./$(LIB_DIR)/win32 -lvlc -lvlccore
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CXXFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CXXFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CXXFLAGS += -D IA32
        endif
    endif
else
	CP_RES = cp -r res $(BUILD_DIR)
	CP = cp
	RM = rm
	RRM = rm -rf
	MKDIRP = mkdir -p
	IGNORE_ERRORS = 2>/dev/null || true
  	SDL2FLAGS=-L$(LIB_DIR)/darwin -lSDL2main -lSDL2# load vlc from lib/darwin
		VLCFLAGS=-L$(LIB_DIR)/darwin -lvlc -lvlccore 
	SLASH=/
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CXXFLAGS += -D LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
				VLC_PLUGINS_TAR=libvlcplugins/darwin.tar.gz
				LDFLAGS += -rpath @executable_path/lib -mmacosx-version-min=10.9
				CXXFLAGS += -D OSX 
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CXXFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CXXFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CXXFLAGS += -D ARM
    endif
endif


.PHONY: all msg clean fullclean

all: msg main

$(BUILD_DIR):
	@$(MKDIRP) $(BUILD_DIR)

plugins:
	tar -xzf $(VLC_PLUGINS_TAR) -C $(BUILD_DIR)
	@echo '--- Unzipped $(VLC_PLUGINS_TAR) to $(BUILD_DIR) ---'

msg:
	@echo '--- C++11 ---'

COMPILE = ${CXX} ${CXXFLAGS} $< ${SDL2FLAGS} ${VLCFLAGS} ${LDFLAGS} -g

main: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(COMPILE) -o $(BUILD_DIR)/$@
	$(CP_RES)

main_universal: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(COMPILE) -arch x86_64 -o $(BUILD_DIR)/$@.x86_64
	$(COMPILE) -arch arm64 -o $(BUILD_DIR)/$@.arm64
	lipo -create $(BUILD_DIR)/$@.x86_64 $(BUILD_DIR)/$@.arm64 -output $(BUILD_DIR)/main
	rm $(BUILD_DIR)/$@.x86_64 $(BUILD_DIR)/$@.arm64
	$(CP_RES)

windows: main | plugins
	$(CP) $(LIB_DIR)$(SLASH)win32$(SLASH)*.dll $(BUILD_DIR)

darwin: main_universal | plugins #Bundle .app
	mkdir -p $(BUILD_DIR)/lib
	cp -r $(LIB_DIR)/darwin/*.dylib $(BUILD_DIR)/lib
	mkdir -p $(BUILD_DIR)/main.app/Contents/MacOS/{lib,plugins}
	cp -r $(BUILD_DIR)/plugins $(BUILD_DIR)/main.app/Contents/MacOS/plugins
	mkdir -p $(BUILD_DIR)/main.app/Contents/Resources
	cp $(BUILD_DIR)/main $(BUILD_DIR)/main.app/Contents/MacOS
	cp -r res $(BUILD_DIR)/main.app/Contents/Resources
	cp Info.plist $(BUILD_DIR)/main.app/Contents
	cp -r $(LIB_DIR)/darwin/*.dylib $(BUILD_DIR)/main.app/Contents/MacOS/lib	


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
