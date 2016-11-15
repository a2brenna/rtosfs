#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <memory>
#include <rtos/object_store.h>
#include <rtos/ref_log.h>
#include <map>

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

struct Timespec {
    uint64_t ts_seconds;
    uint32_t ts_nanos;
};

struct Inode{
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_size;
    Timespec st_atim;
    Timespec st_mtim;
    Timespec st_ctim;
    NODE_TYPE type;
    char ref[32];
};

class Node {

    public:
        Node(const Ref &log, const std::shared_ptr<Object_Store> &backend);

        Inode inode();
        void set_inode(const Inode &inode);

        //Directory ops
        std::map<std::string, Node> list();

        //Symlink ops
        std::string target();

        //File ops

    private:
        std::shared_ptr<Object_Store> _backend;
        Ref _log;
};

class File_System {

    public:
        File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend);

        //Fuse operations
        int getattr(const char *path, struct stat *stbuf);

    private:
        Node _root;
        std::shared_ptr<Object_Store> _backend;

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
        //std::map<std::string, Node> _dir_cache;

        /*
         * Check for path in _inode_cache first.
         *
         * If _sync don't write to _inode_cache.
         * This forces filesystem to always fetch from _backend.
         *
         * If !_sync write to _inode_cache *before* returning ptr to Node.
         */
        Node _fetch_node(const char *path);
};

#endif
