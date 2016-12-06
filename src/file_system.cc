#include "file_system.h"

#include "debug.h"
#include "disk_format.pb.h"

#include <cassert>
#include <sys/types.h>
#include <sys/xattr.h>

#include <boost/algorithm/string.hpp>
#include <deque>
#include <string>

#include <ctgmath>

bool has_access(const Inode &inode, const int mode){
    const auto context = fuse_get_context();
    if(mode & R_OK){
        //Want read permissions
        if ((inode.st_mode & S_IROTH) || (inode.st_mode & S_IRWXO)){
        }
        else if(((inode.st_mode & S_IRGRP) || (inode.st_mode & S_IRWXG)) && (inode.st_gid == context->gid)){
        }
        else if(((inode.st_mode & S_IRUSR) || (inode.st_mode & S_IRWXU)) && (inode.st_uid == context->uid)){
        }
        else{
            throw E_ACCESS();
        }
    }
    if(mode & W_OK){
        //Want write permissions
        if (((inode.st_mode & S_IWOTH) == S_IWOTH) || ((inode.st_mode & S_IRWXO) == S_IRWXO)){
        }
        else if(((inode.st_mode & S_IWGRP) || (inode.st_mode & S_IRWXG)) && (inode.st_gid == context->gid)){
        }
        else if(((inode.st_mode & S_IWUSR) || (inode.st_mode & S_IRWXU)) && (inode.st_uid == context->uid)){
        }
        else{
            throw E_ACCESS();
        }
    }
    if(mode & X_OK){
        //Want exec permissions
        if ((inode.st_mode & S_IXOTH) || (inode.st_mode & S_IRWXO)){
        }
        else if(((inode.st_mode & S_IXGRP) || (inode.st_mode & S_IRWXG)) && (inode.st_gid == context->gid)){
        }
        else if(((inode.st_mode & S_IXUSR) || (inode.st_mode & S_IRWXU)) && (inode.st_uid == context->uid)){
        }
        else{
            throw E_ACCESS();
        }
    }
    return true;
}

timespec get_timespec(const std::chrono::high_resolution_clock::time_point &tp){
    const uint64_t nanos = tp.time_since_epoch().count();
    const uint64_t seconds = nanos / 1000000000;
    timespec r;
    r.tv_sec = seconds;
    r.tv_nsec = nanos % 1000000000;
    return r;
}

Node::Node(const Ref &log, const std::shared_ptr<Object_Store> &backend):
    _log(log.buf(), 32)
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

Ref Node::ref() const{
    return _log;
}

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

std::deque<std::string> decompose_path(const std::string &path){
    return decompose_path(path.c_str());
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
            const auto mask = umask(0);
            umask(mask);

            inode.st_mode = S_IFDIR | 0755;
            inode.type = NODE_DIR;
            std::memcpy(inode.data_ref, root_dir_ref.buf(), 32);
            std::memset(inode.xattr_ref, (char)0, 32);
            inode.st_size = dir_size;

            const timespec current_time = get_timespec(std::chrono::high_resolution_clock::now());
            inode.st_atim = current_time;
            inode.st_mtim = current_time;
            inode.st_ctim = current_time;
            inode.st_uid = getuid();
            inode.st_gid = getgid();
        }

        _root.update_inode(inode);
    }

}

Node File_System::_get_node(const std::deque<std::string> &decomp_path){
    Node current_node = _root;

    for(const auto &entry_name: decomp_path){
        const auto current_inode = current_node.inode();
        if(!has_access(current_inode, X_OK)){
            throw E_ACCESS();
        }

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

        {
            auto dir_path = decompose_path(path);
            if(dir_path.size() > 0){
                dir_path.pop_back();
            }
            const Inode dir_inode = _get_node(dir_path).inode();
            has_access(dir_inode, R_OK);
        }

        //Don't need perms on this inode... only its parent directory, see above
        const Inode inode = _get_inode(path);

        /* st_dev, st_blksize are ignored
         * st_ino is ignored, as we do not support use_ino mount option
        stbuf->st_dev = 0;
        stbuf->blksize = 0;
        stbuf->st_ino = 0;
        stbuf->st_rdev = 0;
        */

        //Note: I don't think st_blocks is meaningful without a block size
        stbuf->st_blocks = ceil(inode.st_size / 512.0);

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
    catch(E_ACCESS e){
        return -EACCES;
    }

}

int File_System::getxattr(const char *path, const char *name, char *value, size_t val_size){
    try{
        std::memset(value, '\0', val_size);

        const Inode inode = _get_inode(path);
        has_access(inode, R_OK);

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
    catch(E_ACCESS e){
        return -EACCES;
    }
}

rtosfs::Directory File_System::_get_dir(const std::deque<std::string> &decomp_path){
    const Inode inode = _get_node(decomp_path).inode();
    if(inode.type != NODE_DIR){
        throw E_NOT_DIR();
    }

    //Need read access to list contents
    has_access(inode, R_OK);

    const std::string serialized_dir = _backend->fetch(Ref(inode.data_ref, 32)).data();
    rtosfs::Directory dir;
    dir.ParseFromString(serialized_dir);
    return dir;
}

rtosfs::Directory File_System::_get_dir(const char *path){
    return _get_dir(decompose_path(path));
}

int File_System::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    (void) offset;
    (void) fi;

    try{
        const std::string dir_path(path);
        const rtosfs::Directory dir = _get_dir(path);
        for(const auto &e: dir.entries()){
            const std::string entry_path = dir_path + "/" + e.name();
            const auto entry_inode = _get_inode(entry_path.c_str());
            //Don't need to check permissions on each node... _get_dir checked permissions on parent

            struct stat st;
            st.st_mode = entry_inode.st_mode;

            if(filler(buf, e.name().c_str(), &st, 0)){
                break;
            }
        }
        return 0;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::create(const char *path, mode_t mode, struct fuse_file_info *fi){
    try{
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

            const auto context = fuse_get_context();
            new_file_inode.st_uid = context->uid;
            new_file_inode.st_gid = context->gid;
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
            rtosfs::Directory dir = _get_dir(decomposed_path);

            //check to see if object already exists
            for(const auto &e: dir.entries()){
                if(e.name() == name){
                    return -(EEXIST);
                }
            }

            //Create directory object with new file entry
            std::string serialized_dir;
            auto new_entry = dir.add_entries();
            new_entry->set_name(name);
            new_entry->set_inode_ref(std::string(new_file_inode_ref.buf(), 32));
            dir.SerializeToString(&serialized_dir);

            //Store new instance of directory object
            const Ref new_dir_ref = Ref();
            const Object new_dir = Object(serialized_dir);
            _backend->store(new_dir_ref, new_dir);

            //Update directory inode and append to inode stack
            Node dir_node = _get_node(decomposed_path);
            Inode dir_inode = dir_node.inode();
            has_access(dir_inode, W_OK);

            std::memcpy(dir_inode.data_ref, new_dir_ref.buf(), 32);
            dir_inode.st_atim = current_time;
            dir_inode.st_mtim = current_time;
            dir_inode.st_size = serialized_dir.size();
            dir_node.update_inode(dir_inode);
            return 0;
        }
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
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

//TODO: Support null tv = set current time
int File_System::utimens(const char *path, const struct timespec tv[2]){
    try{
        Node current_node = _get_node(path);
        Inode i = current_node.inode();

        //Special case, man pages indiciate that you don't necessarily need write perms if you're the owner
        try{
            has_access(i, W_OK);
        }
        catch(E_ACCESS e){
            if(i.st_uid != fuse_get_context()->uid){
                return -EACCES;
            }
        }

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
    catch(E_ACCESS e){
        return -EACCES;
    }
}

//TODO: Support null buf = set current time
int File_System::utime(const char *path, struct utimbuf *buf){
    timespec tv[2];
    tv[0].tv_sec = buf->actime;
    tv[0].tv_nsec = 0;
    tv[1].tv_sec = buf->modtime;
    tv[1].tv_nsec = 0;
    return utimens(path, tv);
}

int File_System::chmod(const char *path, mode_t mode){
    try{
        Node current_node = _get_node(path);
        Inode i = current_node.inode();
        //has_access(i, W_OK);
        if(fuse_get_context()->uid != i.st_uid){
            throw E_ACCESS();
        }

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
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::chown(const char *path, uid_t uid, gid_t gid){
    try{
        Node current_node = _get_node(path);
        Inode i = current_node.inode();
        //has_access(i, W_OK);
        if(fuse_get_context()->uid != i.st_uid){
            throw E_ACCESS();
        }

        //TODO: check for a macro or constant for nobody and nogroup...
        if(uid != 4294967295){
            i.st_uid = uid ;
        }
        if(gid != 4294967295){
            i.st_gid = gid ;
        }

        current_node.update_inode(i);
        return 0;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::open(const char *path, struct fuse_file_info *fi){
    try{
        _get_inode(path);
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
        has_access(i, R_OK);

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
            if( (off_t)file.size() <= off ){
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
    catch(E_ACCESS e){
        return -EACCES;
    }
}

//TODO: Fix this so it uses flags correctly
int File_System::setxattr(const char *path, const char *name, const char *value, size_t size, int flags){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();
        has_access(inode, W_OK);

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
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::removexattr(const char *path, const char *name){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();
        has_access(inode, W_OK);

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
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::truncate(const char *path, off_t off){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();
        has_access(inode, W_OK);

        if(off != inode.st_size){
            //TODO:Replace with Object Store mutation tech?
            std::string file = _backend->fetch(Ref(inode.data_ref, 32), 0, inode.st_size).data();
            file.resize(off);

            Ref new_data_ref = Ref();
            _backend->store(new_data_ref, Object(file));
            std::memcpy(inode.data_ref, new_data_ref.buf(), 32);

            inode.st_size = off;
            node.update_inode(inode);
        }
        return 0;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::write(const char *path, const char *buf, size_t size, off_t off,
                    struct fuse_file_info *fi){
    try{
        Node node = _get_node(path);
        Inode inode = node.inode();
        has_access(inode, W_OK);

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
            std::memcpy(inode.data_ref, new_data_ref.buf(), 32);

            node.update_inode(inode);
            return size;
        }
    }
    catch(E_DNE e){
        return -EIO;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::access(const char *path, int mode){
    try{
        const Inode inode = _get_inode(path);

        if( (mode == F_OK) ){
            return 0;
        }
        else{
            has_access(inode, mode);
            return 0;
        }
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::unlink(const char *path){
    try{
        auto decomposed_path = decompose_path(path);
        Node object_node = _get_node(decomposed_path);

        const std::string name = decomposed_path.back();

        decomposed_path.pop_back();
        Node directory_node = _get_node(decomposed_path);

        Inode inode = directory_node.inode();
        has_access(inode, W_OK);

        if(inode.type != NODE_DIR){
            return -ENOTDIR;
        }
        else{
            rtosfs::Directory dir = _get_dir(decomposed_path);

            for(auto i = dir.entries().begin(); i != dir.entries().end(); i++){
                if(i->name() == name){
                    dir.mutable_entries()->erase(i);

                    const Ref new_dir_ref = Ref();
                    {
                        std::string serialized_dir;
                        dir.SerializeToString(&serialized_dir);
                        const Object empty_file = Object(serialized_dir);
                        _backend->store(new_dir_ref, empty_file);
                    }
                    std::memcpy(inode.data_ref, new_dir_ref.buf(), 32);
                    directory_node.update_inode(inode);

                    //Now reduce link count on Node
                    Inode object_inode = object_node.inode();
                    assert(object_inode.st_nlink > 0);

                    object_inode.st_nlink--;
                    object_node.update_inode(object_inode);

                    return 0;
                }
            }

            return -ENOENT;
        }
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::mkdir(const char *path, mode_t mode){
    try{
        auto decomposed_path = decompose_path(path);
        const std::string new_dir_name = decomposed_path.back();

        decomposed_path.pop_back();
        Node parent_dir_node = _get_node(decomposed_path);

        Inode parent_inode = parent_dir_node.inode();
        has_access(parent_inode, W_OK);

        if(parent_inode.type != NODE_DIR){
            return -ENOTDIR;
        }
        else{
            rtosfs::Directory parent_dir = _get_dir(decomposed_path);

            //Check to see if it already exists
            for(auto i = parent_dir.entries().begin(); i != parent_dir.entries().end(); i++){
                if(i->name() == new_dir_name){
                    return -EEXIST;
                }
            }

            //Does not exist, add it

            const Ref new_dir_log_ref = Ref();
            Node new_dir_node(new_dir_log_ref, _backend);
            Inode new_dir_inode;
            {
                //Make a new empty directory and store it
                const Ref new_dir_ref = Ref();
                {
                    _backend->store(new_dir_ref, Object(""));
                }

                _debug_log() << "New empty directory stored" << std::endl;

                new_dir_inode.st_mode = S_IFDIR | mode;
                new_dir_inode.type = NODE_DIR;
                std::memcpy(new_dir_inode.data_ref, new_dir_ref.buf(), 32);
                std::memset(new_dir_inode.xattr_ref, (char)0, 32);
                //New directories are empty strings in protobuf speak
                new_dir_inode.st_size = 0;
                new_dir_inode.st_nlink = 1;

                const timespec current_time = get_timespec(std::chrono::high_resolution_clock::now());
                new_dir_inode.st_atim = current_time;
                new_dir_inode.st_mtim = current_time;
                new_dir_inode.st_ctim = current_time;
                new_dir_inode.st_uid = fuse_get_context()->uid;
                new_dir_inode.st_gid = fuse_get_context()->gid;
            }
            new_dir_node.update_inode(new_dir_inode);

            //Add entry to new directory in parent directory
            auto new_entry = parent_dir.add_entries();
            {
                new_entry->set_name(new_dir_name);
                new_entry->set_inode_ref(new_dir_log_ref.buf(), 32);
            }

            //Store new instance of parent directory at a new ref
            const Ref new_parent_dir_ref = Ref();
            {
                std::string serialized_parent;
                parent_dir.SerializeToString(&serialized_parent);
                _backend->store(new_parent_dir_ref, Object(serialized_parent));
            }

            //Update parent inode with new parent ref
            std::memcpy(parent_inode.data_ref, new_parent_dir_ref.buf(), 32);
            parent_dir_node.update_inode(parent_inode);
            return 0;
        }
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::rmdir(const char *path){
    try{
        const auto dir = _get_dir(decompose_path(path));
        if(dir.entries().size() != 0){
            return -ENOTEMPTY;
        }
        else{
            return unlink(path);
        }
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::symlink(const char *to, const char *from){
    try{
        if( ((strnlen(to, 4096) >= 4096) || (strnlen(from, 4096) >= 4096)) ){
            return -ENAMETOOLONG;
        }

        const auto path = decompose_path(from);
        const std::string name = path.back();
        auto dir_path = path;
        dir_path.pop_back();

        auto source_dir = _get_dir(dir_path);
        for(const auto &e: source_dir.entries()){
            if(name == e.name()){
                return -EEXIST;
            }
        }

        const std::string dest(to);
        const Ref dest_ref = Ref();
        {
            _backend->store(dest_ref, Object(dest));
        }

        const Ref new_link_log_ref = Ref();
        {
            Inode new_link_inode;
            {
                new_link_inode.st_mode = S_IFLNK | 0777;
                new_link_inode.type = NODE_SYM;
                std::memcpy(new_link_inode.data_ref, dest_ref.buf(), 32);
                std::memset(new_link_inode.xattr_ref, (char)0, 32);
                new_link_inode.st_size = dest.size();
                new_link_inode.st_nlink = 1;

                const timespec current_time = get_timespec(std::chrono::high_resolution_clock::now());
                new_link_inode.st_atim = current_time;
                new_link_inode.st_mtim = current_time;
                new_link_inode.st_ctim = current_time;
                new_link_inode.st_uid = fuse_get_context()->uid;
                new_link_inode.st_gid = fuse_get_context()->gid;
            }

            Node new_link_node(new_link_log_ref, _backend);
            new_link_node.update_inode(new_link_inode);
        }

        auto new_entry = source_dir.add_entries();
        new_entry->set_name(name);
        new_entry->set_inode_ref(std::string(new_link_log_ref.buf(), 32));

        std::string serialized_dir;
        source_dir.SerializeToString(&serialized_dir);
        const auto new_dir_ref = Ref();
        _backend->store(new_dir_ref, serialized_dir);

        Node dir_node = _get_node(dir_path);
        auto dir_inode = dir_node.inode();
        std::memcpy(dir_inode.data_ref, new_dir_ref.buf(), 32);
        dir_node.update_inode(dir_inode);
        return 0;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::readlink(const char *path, char *linkbuf, size_t size){
    try{
        Node link_node = _get_node(path);
        const Inode link_inode = link_node.inode();
        const std::string target = _backend->fetch(Ref(link_inode.data_ref, 32)).data();

        const size_t to_copy = std::min(target.size(), size);
        std::strncpy(linkbuf, target.c_str(), to_copy);

        //readlink(...) standard library call does NOT append a null terminator, but we do cuz libfuse seems to expect it...
        linkbuf[to_copy] = '\0';

        return 0;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

int File_System::link(const char *to, const char *from){
    try{
        if( ((strnlen(to, 4096) >= 4096) || (strnlen(from, 4096) >= 4096)) ){
            return -ENAMETOOLONG;
        }

        const auto path = decompose_path(from);
        const std::string name = path.back();
        auto dir_path = path;
        dir_path.pop_back();

        auto source_dir = _get_dir(dir_path);
        for(const auto &e: source_dir.entries()){
            if(name == e.name()){
                return -EEXIST;
            }
        }


        Node to_node = _get_node(to);
        {
            Inode to_inode = to_node.inode();
            to_inode.st_nlink++;
            to_node.update_inode(to_inode);
        }

        auto new_entry = source_dir.add_entries();
        new_entry->set_name(name);
        new_entry->set_inode_ref(to_node.ref().buf(), 32);

        std::string serialized_dir;
        source_dir.SerializeToString(&serialized_dir);
        const auto new_dir_ref = Ref();
        _backend->store(new_dir_ref, serialized_dir);

        Node dir_node = _get_node(dir_path);
        auto dir_inode = dir_node.inode();
        std::memcpy(dir_inode.data_ref, new_dir_ref.buf(), 32);
        dir_node.update_inode(dir_inode);
        return 0;
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}

std::deque<std::string> get_dir_path(std::deque<std::string> file_path){
    assert(file_path.size() > 0);
    file_path.pop_back();
    return file_path;
}

int File_System::rename(const char *source, const char *dest){
    try{
        const std::string _source(source);
        const std::string _dest(dest);
        if(_source == dest){
            return 0;
        }

        const std::deque<std::string> source_file_path = decompose_path(_source);
        const std::deque<std::string> dest_file_path = decompose_path(_dest);
        const std::deque<std::string> source_dir_path = get_dir_path(source_file_path);
        const std::deque<std::string> dest_dir_path = get_dir_path(dest_file_path);
        const std::string source_file_name = source_file_path.back();
        const std::string dest_file_name = dest_file_path.back();

        (void)source_file_path;
        (void)dest_file_path;

        Node source_dir_node = _get_node(source_dir_path);
        Inode source_dir_inode = source_dir_node.inode();
        has_access(source_dir_inode, W_OK);

        Node dest_dir_node = _get_node(dest_dir_path);
        Inode dest_dir_inode = dest_dir_node.inode();
        has_access(dest_dir_inode, W_OK);

        if(source_dir_path == dest_dir_path){
            (void)dest_dir_path;
            (void)dest_dir_node;
            (void)dest_dir_inode;
            _debug_log() << source_dir_node.ref().base16() << std::endl;

            rtosfs::Directory dir = _get_dir(source_dir_path);
            rtosfs::Directory new_dir;
            (void)source_dir_path;

            Ref file_ref;

            //create new dir
            bool exists = false;
            for(const auto &d: dir.entries()){
                if(d.name() == source_file_name){
                    file_ref = Ref(d.inode_ref().c_str(), 32);
                    exists = true;
                }
                else if(d.name() == dest_file_name){
                    //don't copy
                }
                else{
                    auto n = new_dir.add_entries();
                    n->set_name(d.name());
                    n->set_inode_ref(d.inode_ref());
                }
            }
            if(!exists){
                return -ENOENT;
            }

            //modify new dir
            auto new_dir_ent = new_dir.add_entries();
            new_dir_ent->set_name(dest_file_name);
            new_dir_ent->set_inode_ref(file_ref.buf(), 32);

            //serialize and store source dir
            const Ref new_dir_ref = Ref();
            std::string serialized_dir;
            new_dir.SerializeToString(&serialized_dir);
            _backend->store(new_dir_ref, Object(serialized_dir));

            //update source dir inode
            std::memcpy(source_dir_inode.data_ref, new_dir_ref.buf(), 32);
            source_dir_node.update_inode(source_dir_inode);

            return 0;
        }
        else{
            rtosfs::Directory source_dir = _get_dir(source_dir_path);
            rtosfs::Directory new_source_dir;
            Ref file_ref;
            {
                //modify source dir
                bool exists = false;
                for(const auto &d: source_dir.entries()){
                    if(d.name() == source_file_name){
                        file_ref = Ref(d.inode_ref().c_str(), 32);
                        exists = true;
                    }
                    else{
                        auto n = new_source_dir.add_entries();
                        n->set_name(d.name());
                        n->set_inode_ref(d.inode_ref());
                    }
                }
                if(!exists){
                    return -ENOENT;
                }
            }
            (void)source_dir;

            //modify new dir
            const rtosfs::Directory old_dest_dir = _get_dir(dest_dir_path);
            rtosfs::Directory new_dest_dir;
            {
                for(const auto &e: old_dest_dir.entries()){
                    if(e.name() != dest_file_name){
                        auto n = new_dest_dir.add_entries();
                        n->set_name(e.name());
                        n->set_inode_ref(e.inode_ref());
                    }
                }
                auto new_dir_ent = new_dest_dir.add_entries();
                new_dir_ent->set_name(dest_file_name);
                new_dir_ent->set_inode_ref(file_ref.buf(), 32);
            }

            //serialize and store source dir
            const Ref new_source_dir_ref = Ref();
            {
                std::string serialized_source_dir;
                new_source_dir.SerializeToString(&serialized_source_dir);
                _backend->store(new_source_dir_ref, Object(serialized_source_dir));
            }

            //serialize and store new dir
            const Ref new_dest_dir_ref = Ref();
            {
                std::string serialized_dest_dir;
                new_dest_dir.SerializeToString(&serialized_dest_dir);
                _backend->store(new_dest_dir_ref, Object(serialized_dest_dir));
            }

            //update target dir inode
            std::memcpy(dest_dir_inode.data_ref, new_dest_dir_ref.buf(), 32);
            dest_dir_node.update_inode(dest_dir_inode);
            //update source dir inode
            std::memcpy(source_dir_inode.data_ref, new_source_dir_ref.buf(), 32);
            source_dir_node.update_inode(source_dir_inode);

            return 0;
        }
    }
    catch(E_NOT_DIR e){
        return -ENOTDIR;
    }
    catch(E_DNE e){
        return -ENOENT;
    }
    catch(E_ACCESS e){
        return -EACCES;
    }
}
