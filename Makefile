INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESDTIR=/
PREFIX=/usr

CXX=g++
CXXFLAGS=-D_FILE_OFFSET_BITS=64 -L${LIBRARY_DIR} -I${INCLUDE_DIR} -O2 -g -std=c++14 -fPIC -Wall -Wextra -march=native

all: rtosfs

install: all

rtosfs: src/rtosfs.cc operations.o directory.o
	${CXX} ${CXXFLAGS} -o rtosfs src/rtosfs.cc operations.o directory.o -lfuse -lboost_program_options -lsmplsocket

operations.o: src/operations.cc src/operations.h
	${CXX} ${CXXFLAGS} -c src/operations.cc -o operations.o

directory.o: src/directory.cc src/directory.h
	${CXX} ${CXXFLAGS} -c src/directory.cc -o directory.o

file.o: src/file.cc src/file.h
	${CXX} ${CXXFLAGS} -c src/file.cc -o file.o

clean:
	rm -f rtosfs
	rm -f *.o
	rm -f *.so
	rm -f *.a
