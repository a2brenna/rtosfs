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

class Node {

    public:
        Node(const Ref_Log &ref_log, const std::shared_ptr<Object_Store> &backend);
        rtosfs::Inode inode();

        std::map<std::string, std::shared_ptr<Node>> list();
        std::string target();

    private:
        std::shared_ptr<Object_Store> _backend;
        Ref_Log _ref_log;

};

class File_System {

    public:
        File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend);

        //Fuse operations
        int getattr(const char *path, struct stat *stbuf);

    private:
        Ref_Log _ref_log;
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
        std::map<std::string, std::shared_ptr<Node>> _dir_cache;

        /*
         * Check for path in _inode_cache first.
         *
         * If _sync don't write to _inode_cache.
         * This forces filesystem to always fetch from _backend.
         *
         * If !_sync write to _inode_cache *before* returning ptr to Node.
         */
        std::shared_ptr<Node> _fetch_inode(const char *path);
};

#endif
