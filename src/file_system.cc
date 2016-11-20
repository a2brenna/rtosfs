#include "file_system.h"

#include "debug.h"
#include "disk_format.pb.h"
#include <cassert>

#include <boost/algorithm/string.hpp>
#include <deque>
#include <string>

timespec get_timespec(const std::chrono::high_resolution_clock::time_point &tp){
    const uint64_t nanos = tp.time_since_epoch().count();
    const uint64_t seconds = nanos / 1000000000;
    timespec r;
    r.tv_sec = seconds;
    r.tv_nsec = nanos % 1000000000;
    return r;
}

std::ostream &operator<<(std::ostream &out, const timespec &t){
    out << "seconds: " << t.tv_sec << " nanos: " << t.tv_nsec;
    return out;
}

std::ostream &operator<<(std::ostream &out, const Inode &i){
    out << "st_mode: " << i.st_mode << std::endl;
    out << "st_uid: " << i.st_uid << std::endl;
    out << "st_gid: " << i.st_gid << std::endl;
    out << "st_size: " << i.st_size << std::endl;
    out << "st_atim: " << i.st_atim << std::endl;
    out << "st_mtim: " << i.st_mtim << std::endl;
    out << "st_ctim: " << i.st_ctim << std::endl;
    out << "type: " << i.type;
    return out;
}

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

/*
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
*/

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
            inode.st_mode = S_IFDIR | 0777;
            inode.type = NODE_DIR;
            std::memcpy(inode.data_ref, root_dir_ref.buf(), 32);
            std::memset(inode.xattr_ref, '\0', 32);
            inode.st_size = dir_size;

            const timespec current_time = get_timespec(std::chrono::high_resolution_clock::now());
            inode.st_atim = current_time;
            inode.st_mtim = current_time;
            inode.st_ctim = current_time;
            inode.st_uid = 0;
            inode.st_gid = 0;
        }

        _root.update_inode(inode);
    }
}

std::deque<std::string> decompose_path(const char *path){
    std::deque<std::string> decomposed;
    split(decomposed, path, boost::is_any_of("/"));
    return decomposed;
}

Node File_System::_get_node(const char *path){
    Node current_node = _root;

    const std::deque<std::string> decomp_path = decompose_path(path);
    for(const auto &entry_name: decomp_path){
        //TODO:
        //do directory lookup
    }

    return current_node;
}

int File_System::getattr(const char *path, struct stat *stbuf){
    try{
        std::memset(stbuf, 0, sizeof(struct stat));

        Node node = _get_node(path);
        const Inode inode = node.inode();

        _debug_log() << inode << std::endl;

        //See linux kernel Documents/devices.txt: Major dev id 60 - 63 reserved for
        //Local/Experimental char devices
        //TODO: Use a nonce for the minor device id
        stbuf->st_dev = makedev(60, 1);

        stbuf->st_mode = inode.st_mode;
        stbuf->st_nlink = inode.st_nlink;
        stbuf->st_uid = inode.st_uid;
        stbuf->st_gid = inode.st_gid;

        //Currently read and write a character at a time
        stbuf->st_size = inode.st_size;
        stbuf->st_blksize = 1;
        stbuf->st_blocks = 1;

        stbuf->st_atim = inode.st_atim;
        stbuf->st_mtim = inode.st_mtim;
        stbuf->st_ctim = inode.st_ctim;

        /*
        * UNSUPPORTED
        */
        stbuf->st_ino = 1;
        stbuf->st_rdev = makedev(60,1);

        return 0;
    }
    catch(...){
        return -1;
    }

}
