#include <fuse.h>
#include <boost/program_options.hpp>
#include <string>
#include <iostream>

namespace po = boost::program_options;

static struct fuse_operations xmp_oper = {
/*
    .init       = xmp_init,
    .getattr    = xmp_getattr,
    .access     = xmp_access,
    .readlink   = xmp_readlink,
    .readdir    = xmp_readdir,
    .mknod      = xmp_mknod,
    .mkdir      = xmp_mkdir,
    .symlink    = xmp_symlink,
    .unlink     = xmp_unlink,
    .rmdir      = xmp_rmdir,
    .rename     = xmp_rename,
    .link       = xmp_link,
    .chmod      = xmp_chmod,
    .chown      = xmp_chown,
    .truncate   = xmp_truncate,
    .utimens    = xmp_utimens,
    .open       = xmp_open,
    .read       = xmp_read,
    .write      = xmp_write,
    .statfs     = xmp_statfs,
    .release    = xmp_release,
    .fsync      = xmp_fsync,
    .fallocate  = xmp_fallocate,
    .setxattr   = xmp_setxattr,
    .getxattr   = xmp_getxattr,
    .listxattr  = xmp_listxattr,
    .removexattr	= xmp_removexattr,
*/
};

int main(int argc, char *argv[]){

	std::string RTOSD_UDS;
	std::string FS;

    po::options_description desc("Options");
    desc.add_options()
        ("rtosd-uds", po::value<std::string>(&RTOSD_UDS), "Unix Domain Socket of rtosd")
        ("fs", po::value<std::string>(&FS), "File System to mount")
    ;

    try{
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(...){
        std::cout << desc << std::endl;
        return -1;
    }

	if( (FS.size() == 0) || (RTOSD_UDS.size() == 0) ){
        std::cout << desc << std::endl;
        return -1;
	}

    return 0;

    //return fuse_main(argc, argv, &xmp_oper, NULL);
}
