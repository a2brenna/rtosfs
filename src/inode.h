#ifndef __INODE_H__
#define __INODE_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ostream>

enum NODE_TYPE{
    NODE_DIR,
    NODE_FILE,
    NODE_SYM
};

struct Inode{
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    off_t st_size;
    nlink_t st_nlink;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    NODE_TYPE type;
    char data_ref[32];
    char xattr_ref[32];
};

std::ostream &operator<<(std::ostream &out, const timespec &t);
std::ostream &operator<<(std::ostream &out, const Inode &i);

#endif
