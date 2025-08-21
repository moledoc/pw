
INCLUDES =
DEFINES =
CCFLAGS = 
DEVFLAGS = -g -fsanitize=address
LDFLAGS = -lm
SDLFLAGS =

# TODO: _WIN32 flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
build: dirs
INCLUDES += 
DEFINES += -D_REENTRANT
LDFLAGS += -lpthread
SDLFLAGS = `pkg-config --cflags --libs sdl2 SDL2_ttf`
endif
ifeq ($(UNAME_S),Darwin)
# CCFLAGS += -arch arm64
INCLUDES += -I/opt/homebrew/include
DEFINES += -DOSX -D_THREAD_SAFE
LDFLAGS += -lpthread # -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf
SDLFLAGS = `sdl2-config --libs --cflags` -lSDL2_ttf
endif

dirs:
	mkdir -p bin

build: dirs
	clang -Wall -o ./bin/pwcli ./pwcli.c ${CCFLAGS} ${INCLUDES} ${DEFINES} ${LDFLAGS} ${SDLFLAGS}
	clang -Wall -o ./bin/pwgui ./pwgui.c ${CCFLAGS} ${INCLUDES} ${DEFINES} ${LDFLAGS} ${SDLFLAGS}

dev: dirs
	clang -Wall -o ./bin/pwcli ./pwcli.c ${DEVFLAGS} ${CCFLAGS} ${INCLUDES} ${DEFINES} ${LDFLAGS} ${SDLFLAGS}
	clang -Wall -o ./bin/pwgui ./pwgui.c ${DEVFLAGS} ${CCFLAGS} ${INCLUDES} ${DEFINES} ${LDFLAGS} ${SDLFLAGS}

devrun: dev
	./bin/pwgui -f ./tests/inputs.txt

release: dirs
	clang -Wall -o ./bin/pwcli ./pwcli.c -O3 ${CCFLAGS} ${INCLUDES} ${DEFINES} ${LDFLAGS} ${SDLFLAGS}
	clang -Wall -o ./bin/pwgui ./pwgui.c -O3 ${CCFLAGS} ${INCLUDES} ${DEFINES} ${LDFLAGS} ${SDLFLAGS}

debug-osx: ./bin/pwgui
	lldb ./bin/pwgui -- -f ./tests/inputs.txt

clean:
	rm -rf bin