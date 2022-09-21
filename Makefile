CC=g++
CPPFLAGS=-g -Wpedantic -Wall -Werror $(shell pkg-config --cflags gtk+-3.0)
LIBS=$(shell pkg-config --cflags --libs gtk+-3.0) -lepoxy -lserialport

all: build/cnc

build/cnc: src/main.cpp
	mkdir -p build/
	${CC} ${CPPFLAGS} src/main.cpp -o build/cnc ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
