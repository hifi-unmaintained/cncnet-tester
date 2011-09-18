CC=i586-mingw32msvc-gcc
CFLAGS=-pedantic -Wall -Os -s -Wall
WINDRES=i586-mingw32msvc-windres
DLLTOOL=i586-mingw32msvc-dlltool
LIBS=-Wl,--file-alignment,512 -Wl,--gc-sections -lwininet -lcomctl32 -lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: cncnet-tester

cncnet-tester.rc.o: res/cncnet-tester.rc.in
	sed 's/__REV__/$(REV)/g' res/cncnet-tester.rc.in | $(WINDRES) -o cncnet-tester.rc.o

cncnet-tester: cncnet-tester.rc.o
	$(CC) $(CFLAGS) -mwindows -o cncnet-tester.exe src/main.c src/http.c src/net.c cncnet-tester.rc.o $(LIBS)

clean:
	rm -f cncnet-tester.exe cncnet-tester.rc.o
