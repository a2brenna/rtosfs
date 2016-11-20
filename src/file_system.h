#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <memory>
#include <rtos/object_store.h>
#include <rtos/ref_log.h>
#include <time.h>

#include "disk_format.pb.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

class E_NOT_DIR {};
class E_NOT_SYM {};
class E_NOT_FILE {};

class E_BAD_DIR {};
class E_BAD_SYM {};

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
    char ref[32];
};

std::ostream &operator<<(std::ostream &out, const timespec &t);
std::ostream &operator<<(std::ostream &out, const Inode &i);

class Node {

    public:
        Node(const Ref &log, const std::shared_ptr<Object_Store> &backend);

        Inode inode();
        void update_inode(const Inode &inode);

    private:
        std::shared_ptr<Object_Store> _backend;
        Ref _log;
};

std::map<std::string, Node> dir_list(const Inode &inode);
std::string sym_target(const Inode &inode);

class File_System {

    public:
        File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend);

        //Fuse operations
        int getattr(const char *path, struct stat *stbuf);

    private:
        Node _root;
        std::shared_ptr<Object_Store> _backend;

        Node _get_node(const char *path);

        //TODO:
        //Replace this with a smarter tree structure so we can invalidate
        //directories (and all their contents) by removing its node, thereby
        //removing all references to its contents
        //TODO:
        //Also some sort of linked list for lru cache eviction
        //TODO:
        //some sort of doubly linked tree so we can walk back up to the root,
        //updating last used timestamp/linked list position for cache
        //maintenance?
        //std::map<Ref, Inode> _inode_cache;

};

#endif
