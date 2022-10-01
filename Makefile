CC=g++
CPPFLAGS=-g -Wpedantic -Wall $(shell pkg-config --cflags gtk+-3.0) -I raylib/build/raylib/include/
LIBS=$(shell pkg-config --cflags --libs gtk+-3.0) -lserialport -L raylib/build/raylib/ -lraylib -ldl

all: build/cnc

build/cnc: src/main.cpp
	mkdir -p build/
	${CC} ${CPPFLAGS} $^ -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
