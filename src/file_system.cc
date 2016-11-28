#include "file_system.h"

#include "debug.h"
#include "disk_format.pb.h"

#include <cassert>
#include <sys/types.h>
#include <sys/xattr.h>

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

std::deque<std::string> decompose_path(const char *path){
    //TODO: be less lazy about this, boost split will return empty strings as well
    std::vector<std::string> d;
    std::deque<std::string> decomposed;
    split(d, path, boost::is_any_of("/"));
    for(const auto &e: d){
        if(e.size() != 0){
            decomposed.push_back(e);
        }
    }
    return decomposed;
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

Node File_System::_get_node(const std::deque<std::string> &decomp_path){
    Node current_node = _root;

    for(const auto &entry_name: decomp_path){
        const auto current_inode = current_node.inode();
        //lookup next path element in current_inode directory and then set current_inode = next
        if(current_inode.type != NODE_DIR){
            throw E_NOT_DIR();
        }
        else{
            //get directory corresponding to current_inode
            const Ref serialized_dir_ref(current_inode.data_ref, 32);
            const std::string serialized_dir = _backend->fetch(serialized_dir_ref).data();
            rtosfs::Directory dir;
            dir.ParseFromString(serialized_dir);

            //search directory for next entry
            bool bad_path = true;
            for(const auto &entry: dir.entries()){
                if(entry.name() == entry_name){
                    Node next_node(Ref(entry.inode_ref().c_str(), 32), _backend);
                    current_node = next_node;
                    bad_path = false;
                    break;
                }
            }

            if(bad_path){
                throw E_DNE();
            }
        }
    }

    return current_node;
}

Node File_System::_get_node(const char *path){
    return _get_node(decompose_path(path));
}

Inode File_System::_get_inode(const char *path){
    return _get_node(path).inode();
}

int File_System::getattr(const char *path, struct stat *stbuf){
    try{
        std::memset(stbuf, '\0', sizeof(struct stat));

        const Inode inode = _get_inode(path);

        /* st_dev, st_blksize are ignored
         * st_ino is ignored, as we do not support use_ino mount option
        stbuf->st_dev = 0;
        stbuf->blksize = 0;
        stbuf->st_ino = 0;
        stbuf->st_rdev = 0;
        */

        //Note: I don't think st_blocks is meaningful without a block size
        //stbuf->st_blocks = 0;

        stbuf->st_mode = inode.st_mode;
        stbuf->st_nlink = inode.st_nlink;
        stbuf->st_uid = inode.st_uid;
        stbuf->st_gid = inode.st_gid;

        //Currently read and write a character at a time
        stbuf->st_size = inode.st_size;

        stbuf->st_atim = inode.st_atim;
        stbuf->st_mtim = inode.st_mtim;
        stbuf->st_ctim = inode.st_ctim;

        return 0;
    }
    catch(E_DNE e){
        return -(ENOENT);
    }

}

int File_System::getxattr(const char *path, const char *name, char *value, size_t val_size){
    try{
        std::memset(value, '\0', val_size);

        const Inode inode = _get_inode(path);

        const Ref xattr_ref(inode.xattr_ref, 32);

        const std::string serialized_xattr_dict = _backend->fetch(xattr_ref).data();

        rtosfs::Dictionary xattr_dict;
        xattr_dict.ParseFromString(serialized_xattr_dict);

        for(const auto &entry: xattr_dict.entries()){
            if(std::strncmp(name, entry.name().c_str(), entry.name().size())){
                //Manpage for getxattr states that if the size is 0 return the
                //current size of the attribute in question
                if(val_size == 0){
                    return entry.value().size();
                }
                else{
                    if(entry.value().size() >= val_size){
                        std::strncpy(value, entry.value().c_str(), entry.value().size());
                        return entry.value().size();
                    }
                    else{
                        return -ERANGE;
                    }
                }
            }
            else{
                continue;
            }
        }

        return -ENODATA;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_OBJECT_DNE e){
        return -ENODATA;
    }
}

int File_System::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    (void) offset;
    (void) fi;

    //TODO: THIS IS BROKEN, for some reason path is not set when this function call is made... I assume its trying to use the fuse_file_info *...

    _debug_log() << "Readdir: " << "/" << std::endl;

    const std::string dir_path("/");

    const Inode inode = _get_inode("/");
    if(inode.type != NODE_DIR){
        return EBADF;
    }
    else{
        try{
            const std::string serialized_dir = _backend->fetch(Ref(inode.data_ref, 32)).data();
            rtosfs::Directory dir;
            dir.ParseFromString(serialized_dir);

            for(const auto &e: dir.entries()){
                const std::string entry_path = dir_path + "/" + e.name();
                const auto entry_inode = _get_inode(entry_path.c_str());

                struct stat st;
                st.st_mode = entry_inode.st_mode;

                if(filler(buf, e.name().c_str(), &st, 0)){
                    break;
                }
            }
            return 0;
        }
        catch(...){
            return EBADF;
        }
    }
}

int File_System::create(const char *path, mode_t mode, struct fuse_file_info *fi){

    const timespec current_time = get_timespec(std::chrono::high_resolution_clock::now());

    //create new empty file ref
    const Ref new_file_ref = Ref();
    {
        const Object empty_file = Object("");
        _backend->store(new_file_ref, empty_file);
    }

    //create new empty xattr ref
    const Ref new_xattr_ref = Ref();
    {
        rtosfs::Dictionary xattrs;
        std::string serialized_xattrs;
        xattrs.SerializeToString(&serialized_xattrs);
        const Object empty_xattr = Object(serialized_xattrs);
        _backend->store(new_xattr_ref, empty_xattr);
    }

    //create new empty file inode
    Inode new_file_inode;
    {
        new_file_inode.st_mode = mode;
        new_file_inode.st_nlink = 1;

        //TODO: set this by looking at mode
        new_file_inode.type = NODE_FILE;

        std::memcpy(new_file_inode.data_ref, new_file_ref.buf(), 32);
        std::memcpy(new_file_inode.xattr_ref, new_xattr_ref.buf(), 32);
        new_file_inode.st_size = 0; //we know file is empty

        //TODO: figure out if this should actually be zeroed out...
        new_file_inode.st_atim = current_time;
        new_file_inode.st_mtim = current_time;
        new_file_inode.st_ctim = current_time;

        //TODO: figure out how the hell to set these
        new_file_inode.st_uid = 0;
        new_file_inode.st_gid = 0;
    }

    const Ref new_file_inode_ref = Ref();
    {
        Node new_file_node(new_file_inode_ref, _backend);
        new_file_node.update_inode(new_file_inode);
    }

    //Get Node for directory...
    auto decomposed_path = decompose_path(path);
    if(decomposed_path.size() == 0){
        return -(EEXIST);
    }
    else{
        const std::string name = decomposed_path.back();
        decomposed_path.pop_back();

        //get existing directory
        Node dir_node = _get_node(decomposed_path);
        Inode dir_inode = dir_node.inode();
        std::string serialized_dir = _backend->fetch(Ref(dir_inode.data_ref, 32)).data();
        rtosfs::Directory dir;
        dir.ParseFromString(serialized_dir);

        //check to see if object already exists
        for(const auto &e: dir.entries()){
            if(e.name() == name){
                return -(EEXIST);
            }
        }

        //Create directory object with new file entry
        auto new_entry = dir.add_entries();
        new_entry->set_name(name);
        new_entry->set_inode_ref(std::string(new_file_inode_ref.buf(), 32));
        dir.SerializeToString(&serialized_dir);

        //Store new instance of directory object
        const Ref new_dir_ref = Ref();
        const Object new_dir = Object(serialized_dir);
        _backend->store(new_dir_ref, new_dir);

        //Update directory inode and append to inode stack
        std::memcpy(dir_inode.data_ref, new_dir_ref.buf(), 32);
        dir_inode.st_atim = current_time;
        dir_inode.st_mtim = current_time;
        dir_node.update_inode(dir_inode);
        return 0;
    }
}

//TODO: ACTUALLY IMPLEMENT LOCKING
int File_System::lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *fl){
    if(cmd == F_GETLK){

    }
    else if(cmd == F_SETLK){

    }
    else if(cmd == F_SETLKW){

    }
    else{
        assert(false);
    }

    return 0;
}

int File_System::utimens(const char *path, const struct timespec tv[2]){
    try{
        Node current_node = _get_node(path);
        Inode i = current_node.inode();

        i.st_atim = tv[0];
        i.st_mtim = tv[1];

        current_node.update_inode(i);
        return 0;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
}

int File_System::chmod(const char *path, mode_t mode){
    try{
        Node current_node = _get_node(path);
        Inode i = current_node.inode();

        i.st_mode = mode;

        current_node.update_inode(i);
        return 0;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
}

int File_System::chown(const char *path, uid_t uid, gid_t gid){
    try{
        Node current_node = _get_node(path);
        Inode i = current_node.inode();

        i.st_uid = uid ;
        i.st_gid = gid ;

        current_node.update_inode(i);
        return 0;
    }
    catch(E_DNE e){
        return -1;
    }
}

int File_System::open(const char *path, struct fuse_file_info *fi){
    try{
        const Inode i = _get_inode(path);
        return 0;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
}

int File_System::read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){
    try{
        const Inode i = _get_inode(path);
        const size_t actual_size = i.st_size;

        if(i.type == NODE_DIR){
            return -EISDIR;
        }
        else if(i.type == NODE_SYM){
            return -EBADF;
        }
        else if(off < i.st_size){
            //const auto fetch_size = std::max((size_t)(i.st_size - off), size);
            const Ref data_ref = Ref(i.data_ref, 32);

            //TODO: fix this to do partial fetch when this is supported
            const std::string file = _backend->fetch(data_ref).data();
            if(file.size() <= off){
                return 0;
            }
            else{
                const size_t bytes_to_copy = std::min(size, file.size() - off);
                std::memcpy(buf, &(file[off]), bytes_to_copy);
                return bytes_to_copy;
            }
        }
        else{
            return 0;
        }
    }
    catch(E_DNE e){
        return -EBADF;
    }
    /*
    catch(E_DATA_DNE){
        return 0;
    }
    */
    catch(E_OBJECT_DNE e){
        return -EBADF;
    }
}

int File_System::setxattr(const char *path, const char *name, const char *value, size_t size, int flags){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();

        std::string serialized_xattrs = _backend->fetch(Ref(inode.xattr_ref, 32)).data();
        rtosfs::Dictionary xattrs;
        xattrs.ParseFromString(serialized_xattrs);

        {
            bool set = false;
            for(auto &entry: (*xattrs.mutable_entries())){
                if(std::strncmp(name, entry.name().c_str(), entry.name().size())){
                    entry.set_value(std::string(value, size));
                    set = true;
                }
            }

            if(!set){
                auto new_entry = xattrs.add_entries();
                new_entry->set_name(name);
                new_entry->set_value(std::string(value, size));
            }
        }

        xattrs.SerializeToString(&serialized_xattrs);
        const Ref new_xattr_ref = Ref();
        _backend->store(new_xattr_ref, Object(serialized_xattrs));

        std::memcpy(&(inode.xattr_ref), new_xattr_ref.buf(), 32);
        node.update_inode(inode);

        return 0;
    }
    catch(E_DNE e){
        return -1;
    }
}

int File_System::removexattr(const char *path, const char *name){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();

        std::string serialized_xattrs = _backend->fetch(Ref(inode.xattr_ref, 32)).data();
        rtosfs::Dictionary old_xattrs;
        old_xattrs.ParseFromString(serialized_xattrs);


        //Loop over the xattrs, copy the ones we aren't removing to a new xattrs dict
        rtosfs::Dictionary new_attrs;
        bool dirty = false;
        for(const auto &entry: old_xattrs.entries()){
            if(std::strncmp(name, entry.name().c_str(), entry.name().size())){
                dirty = true;
            }
            else{
                auto new_entry = new_attrs.add_entries();
                new_entry->set_name(entry.name());
                new_entry->set_value(entry.value());
            }
        }

        //If we actually changed the xattrs (by not copying the one we're removing)
        //then write them back out to "disk"
        if(dirty){
            new_attrs.SerializeToString(&serialized_xattrs);
            const Ref new_xattr_ref = Ref();
            _backend->store(new_xattr_ref, Object(serialized_xattrs));

            std::memcpy(&(inode.xattr_ref), new_xattr_ref.buf(), 32);
            node.update_inode(inode);
        }

        return 0;
    }
    catch(E_DNE e){
        return -1;
    }
}

int File_System::truncate(const char *path, off_t off){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();

        if(off != inode.st_size){
            //Replace with Object Store mutation tech?
            std::string file = _backend->fetch(Ref(inode.data_ref, 32), 0, inode.st_size).data();
            file.resize(off);

            Ref new_data_ref = Ref();
            _backend->store(new_data_ref, Object(file));
            std::memcpy(&(inode.data_ref), new_data_ref.buf(), 32);

            inode.st_size = off;
            node.update_inode(inode);
        }
        else{
            return 0;
        }
    }
    catch(E_DNE e){
        return -1;
    }
}

int File_System::write(const char *path, const char *buf, size_t size, off_t off,
                    struct fuse_file_info *fi){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();

        if(off == inode.st_size){
            //A straight append
            _backend->append(Ref(inode.data_ref, 32), buf, size);
            inode.st_size = inode.st_size + size;
            node.update_inode(inode);
            return size;
        }
        else{
            //Rewriting a portion of the file or punching a hole
            std::string current_file = _backend->fetch(Ref(inode.data_ref, 32)).data();
            current_file.resize(off);
            current_file.append(buf, size);

            Ref new_data_ref = Ref();
            _backend->store(new_data_ref, Object(current_file));

            inode.st_size = current_file.size();
            std::memcpy(&(inode.data_ref), new_data_ref.buf(), 32);

            node.update_inode(inode);
            return size;
        }
    }
    catch(E_DNE e){
        return -1;
    }
}
