#include "inode.h"

#include <cassert>
#include <sys/types.h>
#include <sys/xattr.h>

std::ostream &operator<<(std::ostream &out, const timespec &t){
    out << "seconds: " << t.tv_sec << " nanos: " << t.tv_nsec;
    return out;
}

std::ostream &operator<<(std::ostream &out, const Inode &i){
    out << "st_mode: " << i.st_mode << std::endl;
    out << "st_uid: " << i.st_uid << std::endl;
    out << "st_gid: " << i.st_gid << std::endl;
    out << "st_size: " << i.st_size << std::endl;
    out << "st_atim: " << i.st_atim << std::endl;
    out << "st_mtim: " << i.st_mtim << std::endl;
    out << "st_ctim: " << i.st_ctim << std::endl;
    out << "type: " << i.type;
    return out;
}
