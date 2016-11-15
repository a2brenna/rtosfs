#include "file_system.h"

#include <boost/algorithm/string.hpp>
#include <cassert>

Node::Node(const Ref_Log &ref_log, const std::shared_ptr<Object_Store> &backend):
    _ref_log(ref_log)
{
    _backend = backend;
    _dirty = false;
}

File_System::File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend, const MODE &mode):
    _ref_log(Ref(prefix), backend),
    _backend(backend)
{
    _mode = mode;

    try{

    }
    catch(E_OBJECT_DNE e){

    }

}

int File_System::getattr(const char *path, struct stat *stbuf){
    std::memset(stbuf, 0, sizeof(struct stat));

    /*

    //See linux kernel Documents/devices.txt: Major dev id 60 - 63 reserved for
    //Local/Experimental char devices
    //TODO: Use a nonce for the minor device id
    stbuf->st_dev = makedev(60, 1);

    stbuf->st_mode = ;

    //Currently don't support hardlinks
    stbuf->st_nlink = 1;

    stbuf->st_uid = ;
    stbuf->st_gid = ;

    //Currently read and write a character at a time
    stbuf->st_size = ;
    stbuf->st_blksize = 1;
    stbuf->st_blocks = ;

    stbuf->st_atim = ;
    stbuf->st_mtim = ;
    stbuf->st_ctim = ;

     */

/*
 * UNSUPPORTED
    stbuf->st_ino = ;
    stbuf->st_rdev = ;
*/


}
