INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESDTIR=/
PREFIX=/usr

CXX=g++
CXXFLAGS=-D_FILE_OFFSET_BITS=64 -L${LIBRARY_DIR} -I${INCLUDE_DIR} -O2 -g -std=c++14 -fPIC -Wall -Wextra -march=native

all: rtosfs

install: all

rtosfs: src/rtosfs.cc operations.o disk_format.o file_system.o
	${CXX} ${CXXFLAGS} -o rtosfs src/rtosfs.cc operations.o disk_format.o file_system.o -lfuse -lboost_program_options -lsmplsocket -lprotobuf -lrrtos -lsodium

operations.o: src/operations.cc src/operations.h
	${CXX} ${CXXFLAGS} -c src/operations.cc -o operations.o

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
