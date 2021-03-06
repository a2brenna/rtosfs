#include "operations.h"
#include "debug.h"

std::unique_ptr<File_System> fs;

/*
 * const char *path is a path relative to the root of the mountpoint
 */

int rtos_getattr(const char *path, struct stat *stbuf){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    const auto rval = fs->getattr(path, stbuf);
    _debug_log() << "rtos_getattr " << path << " return: " << rval << std::endl;
    return rval;
}

int rtos_readlink(const char *path, char *linkbuf, size_t size){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_readlink " << path << " " << linkbuf << " " << size << std::endl;
    const auto r = fs->readlink(path, linkbuf, size);
    _debug_log() << "rtos_readlink returned : " << r << std::endl;
    return r;
}

int rtos_getdir(const char *path, fuse_dirh_t, fuse_dirfil_t){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_getdir " << path << std::endl;
    return -1;
}

int rtos_mknod(const char *path, mode_t, dev_t){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_mknod " << path << std::endl;
    return -1;
}

int rtos_mkdir(const char *path, mode_t t){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_mkdir " << path << " " << t << std::endl;
    return fs->mkdir(path, t);
}

int rtos_unlink(const char *path){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_unlink " << path << std::endl;
    return fs->unlink(path);
}

int rtos_rmdir(const char *path){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_rmdir " << path << std::endl;
    return fs->rmdir(path);
}

int rtos_symlink(const char *to, const char *from){
    _debug_log() << "rtos_symlink " << from << " " << to << std::endl;
    return fs->symlink(to, from);
}

int rtos_rename(const char *source, const char *dest){
    _debug_log() << "rtos_rename " << source << " " << dest << std::endl;
    fs->rename(source, dest);
}

int rtos_link(const char *to, const char *from){
    _debug_log() << "rtos_link " << from << " " << to << std::endl;
    return fs->link(to, from);
}

int rtos_chmod(const char *path, mode_t mode){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_chmod " << path << std::endl;
    return fs->chmod(path, mode);
}

int rtos_chown(const char *path, uid_t uid, gid_t gid){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_chown " << path << " " << uid << " " << gid << std::endl;
    return fs->chown(path, uid, gid);
}

int rtos_truncate(const char *path, off_t off){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_truncate " << path << std::endl;
    return fs->truncate(path, off);
}

int rtos_utime(const char *path, struct utimbuf *buf){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_utime " << path << std::endl;
    return fs->utime(path, buf);
}

int rtos_open(const char *path, struct fuse_file_info *fi){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_open " << path << std::endl;
    return fs->open(path, fi);
}

int rtos_read(const char *path, char *buf, size_t size, off_t off,
            struct fuse_file_info *fi){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    const auto r = fs->read(path, buf, size, off, fi);
    _debug_log() << "rtos_read " << path << " size: " << size << " off: " << off << " return: " << r << std::endl;
    return r;
}

int rtos_write(const char *path, const char *buf, size_t size, off_t off,
            struct fuse_file_info *fi){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_write " << path << std::endl;
    return fs->write(path,  buf, size, off, fi);
}

int rtos_statfs(const char *path, struct statvfs *){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_statfs " << path << std::endl;
    return -1;

}

int rtos_flush(const char *path, struct fuse_file_info *){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_flush " << path << std::endl;
    return 0;
}

/*
int rtos_release(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_release " << path << std::endl;
    return -1;

}
*/

int rtos_fsync(const char *path, int, struct fuse_file_info *){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_fsync " << path << std::endl;
    return 0;
}

int rtos_setxattr(const char *path, const char *name, const char *value, size_t size, int flags){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_setxattr " << path << std::endl;
    return fs->setxattr(path, name, value, size, flags);
}

int rtos_getxattr(const char *path, const char *name, char *value, size_t size){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    const auto rval = fs->getxattr(path, name, value, size);
    _debug_log() << "rtos_getxattr " << path  << " " << name << " " << size << " return: " << rval << std::endl;
    return rval;

}

int rtos_listxattr(const char *path, char *, size_t){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_listxattr " << path << std::endl;
    return -1;

}

int rtos_removexattr(const char *path, const char *name){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_removexattr " << path << " " << name << std::endl;
    return fs->removexattr(path, name);
}

/*
int rtos_opendir(const char *path, struct fuse_file_info *){
    _debug_log() << "rtos_opendir " << path << std::endl;
    return 1;
}
*/

int rtos_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info *fi){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
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
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_fsync_dir " << path << std::endl;
    return 0;
}

void *rtos_init(struct fuse_conn_info *conn){
    _debug_log() << "rtos_init " << conn << std::endl;
    return nullptr;
}

void rtos_destroy(void *){
    return;
}

int rtos_access(const char *path, int mode){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_access " << path << std::endl;
    return fs->access(path, mode);
}

int rtos_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_create " << path << " mode " << mode << std::endl;
    return fs->create(path, mode, fi);
}

int rtos_ftruncate(const char *path, off_t off, struct fuse_file_info *){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_ftruncate " << path << " " << off << std::endl;
    return fs->truncate(path, off);
}

int rtos_fgetattr(const char *path, struct stat *stbuff, struct fuse_file_info *){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_fgetattr " << path << std::endl;
    return fs->getattr(path, stbuff);
}

int rtos_lock(const char *path, struct fuse_file_info *fi, int cmd,
            struct flock *fl){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_lock " << path << std::endl;
    return fs->lock(path, fi, cmd, fl);
}

int rtos_utimens(const char *path, const struct timespec tv[2]){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_utimens " << path << std::endl;
    return fs->utimens(path, tv);
}

int rtos_bmap(const char *path, size_t blocksize, uint64_t *idx){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_bmap " << path << " " << blocksize << " " << idx << std::endl;
    return -1;
}

int rtos_ioctl(const char *path, int cmd, void *arg,
            struct fuse_file_info *fi, unsigned int flags, void *data){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_ioctl " << path << " " << cmd << " " << arg << " " << fi << " " << flags << " " << data << std::endl;
    return -1;
}

int rtos_poll(const char *path, struct fuse_file_info *fi,
            struct fuse_pollhandle *ph, unsigned int *reventsp){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_poll " << path << " " << fi << " " << ph << " " << reventsp << std::endl;
    return -1;
}

/*
int rtos_write_buf(const char *path, struct fuse_bufvec *buf, off_t off,
            struct fuse_file_info *){
    _debug_log() << "rtos_write_buf " << path << std::endl;
    return -1;

}
*/
int rtos_read_buf(const char *path, struct fuse_bufvec **bufp,
            size_t size, off_t off, struct fuse_file_info *fi){

	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_read_buf " << path << " " << bufp << " " << size << " " << off << " " << fi << std::endl;
    return -1;
}

int rtos_flock(const char *path, struct fuse_file_info *fi, int op){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_flock " << path << " " << fi << " " << op << std::endl;
    return -1;

}

int rtos_fallocate(const char *path, int, off_t, off_t,
            struct fuse_file_info *){
	if(strnlen(path, 4096) >= 4096) return -ENAMETOOLONG;
    _debug_log() << "rtos_fallocate " << path << std::endl;
    return -1;

}
