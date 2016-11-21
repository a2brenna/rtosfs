#include "operations.h"
#include "debug.h"

std::unique_ptr<File_System> fs;

/*
 * const char *path is a path relative to the root of the mountpoint
 */

int rtos_getattr(const char *path, struct stat *stbuf){
    const auto rval = fs->getattr(path, stbuf);
    _debug_log() << "rtos_getattr " << path << " return: " << rval << std::endl;
    return rval;
}

int rtos_readlink(const char *path, char *linkbuf, size_t size){
    _debug_log() << "rtos_realink " << path << std::endl;
    return -1;
}

int rtos_getdir(const char *path, fuse_dirh_t, fuse_dirfil_t){
    _debug_log() << "rtos_getdir " << path << std::endl;
    return -1;
}

int rtos_mknod(const char *path, mode_t, dev_t){
    _debug_log() << "rtos_mknod " << path << std::endl;
    return -1;

}

int rtos_mkdir(const char *path, mode_t){
    _debug_log() << "rtos_mkdir " << path << std::endl;
    return -1;
}

int rtos_unlink(const char *path){
    _debug_log() << "rtos_unlink " << path << std::endl;
    return -1;
}

int rtos_rmdir(const char *path){
    _debug_log() << "rtos_rmdir " << path << std::endl;
    return -1;
}

int rtos_symlink(const char *path, const char *){
    _debug_log() << "rtos_symlink " << path << std::endl;
    return -1;
}

int rtos_rename(const char *path, const char *){
    _debug_log() << "rtos_rename " << path << std::endl;
    return -1;
}

int rtos_link(const char *path, const char *){
    _debug_log() << "rtos_link " << path << std::endl;
    return -1;

}

int rtos_chmod(const char *path, mode_t){
    _debug_log() << "rtos_chmod " << path << std::endl;
    return -1;

}

int rtos_chown(const char *path, uid_t, gid_t){
    _debug_log() << "rtos_chown " << path << std::endl;
    return -1;

}

int rtos_truncate(const char *path, off_t){
    _debug_log() << "rtos_truncate " << path << std::endl;
    return -1;

}

int rtos_utime(const char *path, struct utimbuf *){
    _debug_log() << "rtos_utime " << path << std::endl;
    return -1;

}

int rtos_open(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_open " << path << std::endl;
    return -1;

}

int rtos_read(const char *path, char *, size_t, off_t,
            struct fuse_file_info *){
    _debug_log() << "rtos_read " << path << std::endl;
    return -1;

}

int rtos_write(const char *path, const char *, size_t, off_t,
            struct fuse_file_info *){
    _debug_log() << "rtos_write " << path << std::endl;
    return -1;

}

int rtos_statfs(const char *path, struct statvfs *){
    _debug_log() << "rtos_statfs " << path << std::endl;
    return -1;

}

int rtos_flush(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_flush " << path << std::endl;
    return -1;

}

int rtos_release(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_release " << path << std::endl;
    return -1;

}

int rtos_fsync(const char *path, int, struct fuse_file_info *){
    _debug_log() << "rtos_fsync " << path << std::endl;
    return -1;

}

int rtos_setxattr(const char *path, const char *, const char *, size_t, int){
    _debug_log() << "rtos_setxattr " << path << std::endl;
    return -1;

}

int rtos_getxattr(const char *path, const char *name, char *value, size_t size){
    const auto rval = fs->getxattr(path, name, value, size);
    _debug_log() << "rtos_getxattr " << path  << " " << name << " " << size << " return: " << rval << std::endl;
    return rval;

}

int rtos_listxattr(const char *path, char *, size_t){
    _debug_log() << "rtos_listxattr " << path << std::endl;
    return -1;

}

int rtos_removexattr(const char *path, const char *){
    _debug_log() << "rtos_removexattr " << path << std::endl;
    return -1;

}

/*
int rtos_opendir(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_opendir " << path << std::endl;
    return 1;
}
*/

int rtos_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info *fi){
    _debug_log() << "rtos_readdir " << path << std::endl;
    return fs->readdir(path, buf, filler, offset, fi);
}

/*
int rtos_releasedir(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_releasedir " << path << std::endl;
    return 0;

}
*/

int rtos_fsync_dir(const char *path, int, struct fuse_file_info *){
    _debug_log() << "rtos_fsync_dir " << path << std::endl;
    return -1;

}

void *rtos_init(struct fuse_conn_info *conn){
    _debug_log() << "rtos_init " << conn << std::endl;
}

void rtos_destroy(void *){

}

int rtos_access(const char *path, int){
    _debug_log() << "rtos_access " << path << std::endl;
    return -1;

}

int rtos_create(const char *path, mode_t, struct fuse_file_info *){
    _debug_log() << "rtos_create " << path << std::endl;
    return -1;

}

int rtos_ftruncate(const char *path, off_t, struct fuse_file_info *){
    _debug_log() << "rtos_ftruncate " << path << std::endl;
    return -1;

}

int rtos_fgetattr(const char *path, struct stat *, struct fuse_file_info *){
    _debug_log() << "rtos_fgetattr " << path << std::endl;
    return -1;

}

int rtos_lock(const char *path, struct fuse_file_info *, int cmd,
            struct flock *){
    _debug_log() << "rtos_lock " << path << std::endl;
    return -1;

}

int rtos_utimens(const char *path, const struct timespec tv[2]){
    _debug_log() << "rtos_utimens " << path << std::endl;
    return -1;

}

int rtos_bmap(const char *path, size_t blocksize, uint64_t *idx){
    _debug_log() << "rtos_bmap " << path << std::endl;
    return -1;

}

int rtos_ioctl(const char *path, int cmd, void *arg,
            struct fuse_file_info *, unsigned int flags, void *data){
    _debug_log() << "rtos_ioctl " << path << std::endl;
    return -1;

}

int rtos_poll(const char *path, struct fuse_file_info *,
            struct fuse_pollhandle *ph, unsigned *reventsp){

    _debug_log() << "rtos_poll " << path << std::endl;
    return -1;
}

int rtos_write_buf(const char *path, struct fuse_bufvec *buf, off_t off,
            struct fuse_file_info *){
    _debug_log() << "rtos_write_buf " << path << std::endl;
    return -1;

}

int rtos_read_buf(const char *path, struct fuse_bufvec **bufp,
            size_t size, off_t off, struct fuse_file_info *){

    _debug_log() << "rtos_read_buf " << path << std::endl;
    return -1;
}

int rtos_flock(const char *path, struct fuse_file_info *, int op){
    _debug_log() << "rtos_flock " << path << std::endl;
    return -1;

}

int rtos_fallocate(const char *path, int, off_t, off_t,
            struct fuse_file_info *){
    _debug_log() << "rtos_fallocate " << path << std::endl;
    return -1;

}
