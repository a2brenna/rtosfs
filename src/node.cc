#include "node.h"
#include "disk_format.pb.h"

struct Timestamped_Ref {
    uint64_t timestamp;
    char reference[32];
};

Node::Node() : Node(0,0,0){
}

Node::Node(const uint32_t &uid, const uint32_t &gid, const uint64_t &status){
    _uid = uid;
    _gid = gid;
    _status = status;
}

Directory::Directory(const Ref &ref, std::shared_ptr<Object_Store> backend){
    _ref = ref;
    _backend = backend;

    try{
        struct Timestamped_Ref super_ref;
        _backend->fetch_tail(_ref, sizeof(Timestamped_Ref), (char *)&super_ref);
        Ref latest_dir(super_ref.reference, 32);

        const std::string serialized_directory = _backend->fetch(latest_dir).data();
        rtosfs::Directory directory;
        directory.ParseFromString(serialized_directory);

        for(const auto &n: directory.nodes()){
            const std::string name = n.name();
            assert(_contents.find(name) == _contents.end());

            const rtosfs::Metadata metadata = n.metadata();

            std::shared_ptr<Node> new_node;
            if(n.file_ref().size() == 32){
                new_node = std::shared_ptr<Node>(new File(Ref(n.file_ref().c_str(), 32), metadata.uid(), metadata.gid(), metadata.status()));
            }
            else if(n.symlink().size() > 0){
                new_node = std::shared_ptr<Node>(new Symlink(n.symlink(), metadata.uid(), metadata.gid(), metadata.status()));
            }
            else{
                //directory, handle later
                assert(false);
            }

            _contents.insert(std::pair<std::string, std::shared_ptr<Node>>(name, new_node));
        }
    }
    catch(E_OBJECT_DNE e){
        //No object exists, empty directory
    }

}

File::File(const Ref &ref, const uint32_t &uid, const uint32_t &gid, const uint64_t &status) :
    Node(uid, gid, status)
{
    _ref = ref;
}

Symlink::Symlink(const std::string &target, const uint32_t &uid, const uint32_t &gid, const uint64_t &status) :
    Node(uid, gid, status)
{
    _target = target;
}
