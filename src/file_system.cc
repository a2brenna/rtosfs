#include "file_system.h"

#include "disk_format.pb.h"
#include <boost/algorithm/string.hpp>
#include <cassert>

const mode_t PERMISSIVE_MODE = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

Node::Node(const Ref &log, const std::shared_ptr<Object_Store> &backend):
    _log(log)
{
    _backend = backend;
}

Inode Node::inode(){
    struct Inode i;
    _backend->fetch_tail(_log, sizeof(Inode), (char *)(&i));
    return i;
}

void Node::update_inode(const Inode &inode){
    _backend->append(_log, (const char *)(&inode), sizeof(Inode));
}

std::map<std::string, Node> Node::list(){
    std::map<std::string, Node> result;
    const Inode current_inode = inode();

    if(current_inode.type == NODE_DIR){
        const Ref dir_ref(current_inode.ref, 32);
        const std::string serialized_dir = _backend->fetch(dir_ref).data();
        rtosfs::Directory current_directory;
        current_directory.ParseFromString(serialized_dir);
        for(const auto &e: current_directory.entries()){
            if(result.count(e.name()) == 0){
                const Ref inode_log(e.inode_ref().c_str(), 32);

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

Timespec get_timespec(const std::chrono::high_resolution_clock::time_point &tp){
    const uint64_t nanos = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    const uint64_t seconds = nanos / 1000000000;
    Timespec r;
    r.ts_seconds = seconds;
    r.ts_nanos = nanos % 1000000000;
    return r;
}

File_System::File_System(const std::string &prefix, const std::shared_ptr<Object_Store> &backend):
    _root(Ref(prefix), backend),
    _backend(backend)
{
    try{
        _backend->fetch_tail(Ref(prefix), sizeof(Inode));
    }
    catch(E_OBJECT_DNE e){ //empty filesystem
        //write new empty directory
        const Ref root_dir_ref = Ref();
        size_t dir_size;
        {
            const rtosfs::Directory dir;
            std::string serialized_dir;
            dir.SerializeToString(&serialized_dir);
            const Object dir_object = Object(serialized_dir);
            _backend->store(root_dir_ref, dir_object);
            dir_size = serialized_dir.size();
        }

        Inode inode;
        {
            inode.st_mode = PERMISSIVE_MODE;
            inode.type = NODE_DIR;
            std::memcpy(inode.ref, root_dir_ref.buf(), 32);
            inode.st_size = dir_size;

            const Timespec current_time = get_timespec(std::chrono::high_resolution_clock::now());
            inode.st_atim = current_time;
            inode.st_mtim = current_time;
            inode.st_ctim = current_time;
            inode.st_uid = 0;
            inode.st_gid = 0;
        }

        _root.update_inode(inode);
    }
}

Node File_System::_fetch_node(const char *path){
    return _root;
}

Node traverse(const char *path, const Node &node){

}

int File_System::getattr(const char *path, struct stat *stbuf){
    std::memset(stbuf, 0, sizeof(struct stat));

    Node node = _fetch_node(path);

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
