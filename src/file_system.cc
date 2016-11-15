#include "file_system.h"

#include "disk_format.pb.h"
#include <boost/algorithm/string.hpp>
#include <cassert>

Node::Node(const Ref_Log &ref_log, const std::shared_ptr<Object_Store> &backend):
    _ref_log(ref_log)
{
    _backend = backend;
}

rtosfs::Inode Node::inode(){
    const auto top = _ref_log.latest_object();
    rtosfs::Inode current_inode;
    current_inode.ParseFromString(top.second.data());
    return current_inode;
}

std::map<std::string, Node> Node::list(){
    std::map<std::string, Node> result;
    const auto current_inode = inode();

    if(current_inode.type() == rtosfs::Inode::DIR){
        const Ref dir_ref(current_inode.ref().c_str(), 32);
        const std::string serialized_dir = _backend->fetch(dir_ref).data();
        rtosfs::Directory current_directory;
        current_directory.ParseFromString(serialized_dir);
        for(const auto &e: current_directory.entries()){
            if(result.count(e.name()) == 0){
                const Ref inode_log_ref(e.inode_ref().c_str(), 32);
                const Ref_Log inode_log(inode_log_ref, _backend);

                result.insert(std::pair<std::string, Node>(e.name(), Node(inode_log, _backend)));
            }
            else{
                throw E_BAD_DIR();
            }
        }
    }
    else{
        throw E_NOT_DIR();
    }

    return result;
}

File_System::File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend):
    _ref_log(Ref(prefix), backend),
    _backend(backend)
{
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
