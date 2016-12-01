INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESDTIR=/
PREFIX=/usr

CXX=g++
CXXFLAGS=-D_FILE_OFFSET_BITS=64 -L${LIBRARY_DIR} -I${INCLUDE_DIR} -O2 -g -std=c++14 -fPIC -Wall -Wextra -march=native

all: rtosfs rtosfsctl

install: all

rtosfsctl: src/rtosfsctl.cc disk_format.o inode.o
	${CXX} ${CXXFLAGS} -o rtosfsctl src/rtosfsctl.cc disk_format.o inode.o -lboost_program_options -lsmplsocket -lprotobuf -lrrtos -lsodium

rtosfs: src/rtosfs.cc operations.o disk_format.o file_system.o debug.o inode.o
	${CXX} ${CXXFLAGS} -o rtosfs src/rtosfs.cc operations.o disk_format.o file_system.o debug.o inode.o -lfuse -lboost_program_options -lsmplsocket -lprotobuf -lrrtos -lsodium

inode.o: src/inode.cc src/inode.h
	${CXX} ${CXXFLAGS} -c src/inode.cc -o inode.o

operations.o: src/operations.cc src/operations.h
	${CXX} ${CXXFLAGS} -c src/operations.cc -o operations.o

debug.o: src/debug.cc src/debug.h
	${CXX} ${CXXFLAGS} -c src/debug.cc -o debug.o

file_system.o: src/file_system.cc src/file_system.h src/disk_format.pb.h
	${CXX} ${CXXFLAGS} -c src/file_system.cc -o file_system.o

disk_format.o: src/disk_format.pb.h
	${CXX} ${CXXFLAGS} -c src/disk_format.pb.cc -o disk_format.o

src/disk_format.pb.h: disk_format.proto
	protoc --cpp_out=src/ disk_format.proto

clean:
	rm -f rtosfs
	rm -f *.o
	rm -f *.so
	rm -f *.a
