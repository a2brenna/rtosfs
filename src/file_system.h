#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <memory>
#include <deque>
#include <string>
#include <rtos/object_store.h>
#include <rtos/ref_log.h>
#include <time.h>
#include <utime.h>

#include "disk_format.pb.h"
#include "inode.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

class E_NOT_DIR {};
class E_NOT_SYM {};
class E_NOT_FILE {};

class E_BAD_DIR {};
class E_BAD_SYM {};
class E_DNE {};
class E_ACCESS {};

class Node {

    public:
        Node(const Ref &log, const std::shared_ptr<Object_Store> &backend);

        Inode inode();
        void update_inode(const Inode &inode);
        Ref ref() const;

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
        int getxattr(const char *path, const char *name, char *value, size_t size);
        int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
        int create(const char *path, mode_t mode, struct fuse_file_info *fi);
        int lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *fl);
        int utimens(const char *path, const struct timespec tv[2]);
        int utime(const char *path, struct utimbuf *buf);
        int chown(const char *path, uid_t uid, gid_t gid);
        int chmod(const char *path, mode_t mode);
        int open(const char *path, struct fuse_file_info *fi);
        int read(const char *path, char *buf, size_t size, off_t off,
                            struct fuse_file_info *fi);
        int setxattr(const char *path, const char *name, const char *value, size_t size, int flags);
        int removexattr(const char *path, const char *name);
        int truncate(const char *path, off_t off);
        int write(const char *path, const char *buf, size_t size, off_t off,
                            struct fuse_file_info *fi);
        int access(const char *path, int mode);
        int unlink(const char *path);
        int mkdir(const char *path, mode_t t);
        int rmdir(const char *path);
        int symlink(const char *to, const char *from);
        int readlink(const char *path, char *linkbuf, size_t size);
        int link(const char *to, const char *from);
        int rename(const char *source, const char *dest);


    private:
        Node _root;
        std::shared_ptr<Object_Store> _backend;

        Node _get_node(const char *path);
        Node _get_node(const std::deque<std::string> &decomp_path);
        Inode _get_inode(const char *path);
        rtosfs::Directory _get_dir(const char *path);
        rtosfs::Directory _get_dir(const std::deque<std::string> &decomp_path);

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
