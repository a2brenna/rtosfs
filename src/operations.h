#ifndef __OPERATIONS_H__
#define __OPERATIONS_H__

#include <memory>
#include "file_system.h"

extern std::unique_ptr<File_System> fs;

#define FUSE_USE_VERSION 26
#include <fuse.h>

/*Can return:
 * 0: On success
 *  -ENOENT: file/link/dir does not exist
 *  -ENOTDIR: a non-leaf component of the path is not a directory
 */
int rtos_getattr(const char *path, struct stat *);

int rtos_readlink(const char *, char *, size_t);
int rtos_getdir(const char *, fuse_dirh_t, fuse_dirfil_t);
int rtos_mknod(const char *, mode_t, dev_t);
int rtos_mkdir(const char *, mode_t);
int rtos_unlink(const char *);
int rtos_rmdir(const char *);
int rtos_symlink(const char *, const char *);
int rtos_rename(const char *, const char *);
int rtos_link(const char *, const char *);
int rtos_chmod(const char *, mode_t);
int rtos_chown(const char *, uid_t, gid_t);
int rtos_truncate(const char *, off_t);
int rtos_utime(const char *, struct utimbuf *);

/*Can return:
 * 0: On success
 *  -ENOENT: file/link/dir does not exist
 *  -ENOTDIR: a non-leaf component of the path is not a directory
 */
int rtos_open(const char *, struct fuse_file_info *);
int rtos_read(const char *, char *, size_t, off_t,
            struct fuse_file_info *);
int rtos_write(const char *, const char *, size_t, off_t,
            struct fuse_file_info *);
int rtos_statfs(const char *, struct statvfs *);
int rtos_flush(const char *, struct fuse_file_info *);
//int rtos_release(const char *, struct fuse_file_info *);
int rtos_fsync(const char *, int, struct fuse_file_info *);
int rtos_setxattr(const char *, const char *, const char *, size_t, int);

/*Can return:
 * 0: On success
 *  -ENOENT: file/link/dir does not exist
 *  -ENOTDIR: a non-leaf component of the path is not a directory
 *  -ERANGE: value size (as given by val_size) is not large enough to hold the xattr
 */
int rtos_getxattr(const char *path, const char *name, char *value, size_t val_size);
int rtos_listxattr(const char *, char *, size_t);
int rtos_removexattr(const char *, const char *);
//int rtos_opendir(const char *, struct fuse_file_info *);
int rtos_readdir(const char *, void *, fuse_fill_dir_t, off_t,
        struct fuse_file_info *);
//int rtos_releasedir(const char *, struct fuse_file_info *);
int rtos_fsync_dir(const char *, int, struct fuse_file_info *);
void *rtos_init(struct fuse_conn_info *conn);
void rtos_destroy(void *);
int rtos_access(const char *path, int mode);
int rtos_create(const char *, mode_t, struct fuse_file_info *);
int rtos_ftruncate(const char *, off_t, struct fuse_file_info *);
int rtos_fgetattr(const char *, struct stat *, struct fuse_file_info *);
int rtos_lock(const char *, struct fuse_file_info *, int cmd,
            struct flock *);
int rtos_utimens(const char *, const struct timespec tv[2]);
int rtos_bmap(const char *, size_t blocksize, uint64_t *idx);
int rtos_ioctl(const char *, int cmd, void *arg,
            struct fuse_file_info *, unsigned int flags, void *data);
int rtos_poll(const char *, struct fuse_file_info *,
            struct fuse_pollhandle *ph, unsigned *reventsp);
//int rtos_write_buf(const char *, struct fuse_bufvec *buf, off_t off,
//            struct fuse_file_info *);
int rtos_read_buf(const char *, struct fuse_bufvec **bufp,
            size_t size, off_t off, struct fuse_file_info *);
int rtos_flock(const char *, struct fuse_file_info *, int op);
int rtos_fallocate(const char *, int, off_t, off_t,
            struct fuse_file_info *);

static struct fuse_operations rtos_ops {
	.getattr = rtos_getattr,
	.readlink = rtos_readlink,
	.getdir = rtos_getdir,
	.mknod = rtos_mknod,
	.mkdir = rtos_mkdir,
	.unlink = rtos_unlink,
	.rmdir = rtos_rmdir,
	.symlink = rtos_symlink,
	.rename = rtos_rename,
	.link = rtos_link,
	.chmod = rtos_chmod,
	.chown = rtos_chown,
	.truncate = rtos_truncate,
	.utime = rtos_utime,
	.open = NULL,
	.read = rtos_read,
	.write = rtos_write,
	.statfs = rtos_statfs,
	.flush = rtos_flush,
	.release = NULL,
	.fsync = rtos_fsync,
	.setxattr = rtos_setxattr,
	.getxattr = rtos_getxattr,
	.listxattr = rtos_listxattr,
	.removexattr = rtos_removexattr,
	.opendir = NULL,
	.readdir = rtos_readdir,
	.releasedir = NULL,
	.fsyncdir = rtos_fsync_dir,
	.init  = rtos_init,
	.destroy = rtos_destroy,
	.access  = rtos_access,
	.create  = rtos_create,
	.ftruncate = rtos_ftruncate,
	.fgetattr  = rtos_fgetattr,
	.lock = rtos_lock,
	.utimens  = rtos_utimens,
	.bmap = rtos_bmap,
    .flag_nullpath_ok = 0,
    //TODO: re-enable this
    .flag_nopath = 0,
    //TODO: re-enable this
    .flag_utime_omit_ok = 0,
    .flag_reserved = 29,
	.ioctl = rtos_ioctl,
	.poll = rtos_poll,
	.write_buf = NULL,
	.read_buf = NULL,
	.flock = rtos_flock,
	.fallocate = rtos_fallocate,
};

#endif
