# Raylib Makefile

PROJECT_NAME = game
CC = g++
LIB_PATH = ./lib
INCLUDE_PATH = ./src/include/ -I ./lib/include/
LIBS = -ltinyxml2 -lraylib -lopengl32 -lgdi32 -lwinmm
PROJECT_FILES = src/*.cpp
RESOURCES_FOLDER = ./resources/

# --------------- Desktop --------------- #

DESKTOP_FLAGS = -Wall -Wno-missing-braces

DESKTOP_ARGS = $(DESKTOP_FLAGS) -I $(INCLUDE_PATH) -L $(LIB_PATH) $(LIBS)

game: debug.o game.o tractor.o ui.o shop.o base.o
	$(CC) -o $(PROJECT_NAME).exe debug.o game.o tractor.o ui.o shop.o base.o $(DESKTOP_ARGS)

debug.o: src/debug.cpp src/include/debug.h
	$(CC) -c src/debug.cpp $(DESKTOP_ARGS)

game.o: src/game.cpp src/include/game.h src/include/debug.h
	$(CC) -c src/game.cpp $(DESKTOP_ARGS)

tractor.o: src/tractor.cpp src/include/tractor.h src/include/debug.h
	$(CC) -c src/tractor.cpp $(DESKTOP_ARGS)

ui.o: src/ui.cpp src/include/ui.h src/include/debug.h
	$(CC) -c src/ui.cpp $(DESKTOP_ARGS)

shop.o: src/shop.cpp src/include/shop.h src/include/debug.h
	$(CC) -c src/shop.cpp $(DESKTOP_ARGS)

base.o: src/base.cpp src/include/base.h src/include/debug.h
	$(CC) -c src/base.cpp $(DESKTOP_ARGS)

# --------------- WEB --------------- #

WEB_FLAGS = -std=c++17 -Wall -D_DEFAULT_SOURCE -Wno-missing-braces -s -O1 -Os -s USE_GLFW=3 -s TOTAL_MEMORY=16777216 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY
WEB_FLAGS += --shell-file ./lib/web/shell.html

# WEB_FLAGS += -s ASSERTIONS=1 # --profiling # <-- For Debug (takes too long)

ifneq ($(RESOURCES_FOLDER), NONE)
	WEB_FLAGS += --preload-file $(RESOURCES_FOLDER)
endif

WEB_LIB_PATH = $(LIB_PATH)/web/
WEB_LIBS = -ltinyxml2 -lraylib
WEB_COMMAND = em++ -o index.html $(PROJECT_FILES) -I $(INCLUDE_PATH) -L $(WEB_LIB_PATH) $(WEB_FLAGS) $(WEB_LIBS) -D PLATFORM_WEB

web:
	$(WEB_COMMAND)

# --------------- Clean --------------- #

clean:
	rm $(PROJECT_NAME).exe index.data *.wasm *.js *.html

clean-all:
	rm $(PROJECT_NAME).exe index.data *.wasm *.js *.html *.o *.out

# --------------- Info --------------- #

# -- Desktop Compiler Flags
#  -O<level> 0-3        > defines optimization level
#  -g                   > include debug information on compilation
#  -s                   > strip unnecessary data from build -> do not use in debug builds
#  -Wall                > turns on most, but not all, compiler warnings
#  -std=c++14           > C++ standard
#  -Wno-missing-braces  > ignore invalid warning (GCC bug 53119)
#  -D_DEFAULT_SOURCE    > use with -std=c99 on Linux and PLATFORM_WEB, required for timespec

# --  Web Compiler Flags
# -Os                        		> size optimization
# -O2                        		> optimization level 2, if used, also set --memory-init-file 0
# -s USE_GLFW=3              		> Use glfw3 library (context/input management)
# -s ALLOW_MEMORY_GROWTH=1   		> to allow memory resizing -> WARNING: Audio buffers could FAIL!
# -s TOTAL_MEMORY=16777216   		> to specify heap memory size (default = 16MB)
# -s USE_PTHREADS=1          		> multithreading support
# -s WASM=0                  		> disable Web Assembly, emitted by default
# -s EMTERPRETIFY=1          		> enable emscripten code interpreter (very slow)
# -s EMTERPRETIFY_ASYNC=1    		> support synchronous loops by emterpreter
# -s FORCE_FILESYSTEM=1      		> force filesystem to load/save files data
# -s ASSERTIONS=1            		> enable runtime checks for common memory allocation errors (-O1 and above turn it off)
# -s ASYNCIFY=1                	> enable async features
# -s TOTAL_STACK=32MB               > changes stack size
# --profiling                		> include information for code profiling
# --memory-init-file 0       		> to avoid an external memory initialization code file (.mem)
# --preload-file "assets-folder"   	> specify a resources folder for data compilation
