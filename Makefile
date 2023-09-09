CWD := $(shell pwd)

.PHONY: static install build_dir

all: libxcli.so static 

libxcli.so: ${CWD}/src/xcli.c build_dir
	@echo "> Building shared library..."
	gcc -std=c99 -shared -fPIC -Wall -Wextra -Wpedantic $< -o ${CWD}/build/$@

static: ${CWD}/src/xcli.c build_dir
	@echo "> Building static library..."
	gcc -std=c99 -c -Wall -Wextra -Wpedantic $< -o ./xcli.o
	ar cr ${CWD}/build/libxcli.a ./xcli.o
	rm ./xcli.o

install: ${CWD}/build/libxcli.so ${CWD}/include/xcli.h
	ln -sf ${CWD}/build/libxcli.so /usr/local/lib/libxcli.so
	ln -sf ${CWD}/include/xcli.h /usr/local/include/xcli.h

build_dir:
	mkdir -p ${CWD}/build
