#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <memory>
#include <rtos/object_store.h>
#include <rtos/ref_log.h>
#include <map>

#define FUSE_USE_VERSION 26
#include <fuse.h>

class Node {

    public:
        Node(const Ref_Log &ref_log, const std::shared_ptr<Object_Store> &backend);

    private:
        Ref_Log _ref_log;
        std::shared_ptr<Object_Store> _backend;
        bool _dirty;

        virtual void sync() = 0;

};

class Directory : public Node{

    private:
        std::map<std::string, std::shared_ptr<Node>> _nodes;

};

class File : public Node{

};

class Symlink : public Node{

    private:
        std::string _target;

};

enum MODE{
    SINGLE_USER,
    MULTI_USER
};

class File_System {

    public:
        File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend, const MODE &mode);

        //Fuse operations
        int getattr(const char *path, struct stat *stbuf);

    private:
        Ref_Log _ref_log;
        std::shared_ptr<Object_Store> _backend;
        MODE _mode = MULTI_USER;

        std::map<std::string, std::shared_ptr<Node>> _inode_cache;

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
