#ifndef __NODE_H__
#define __NODE_H__

#include <memory>
#include <rtos/object_store.h>
#include <rtos/types.h>
#include <map>
#include <string>

class Node {

    public:
        Node();
        Node(const uint32_t &uid, const uint32_t &gid, const uint64_t &status);

    private:
        uint32_t _uid;
        uint32_t _gid;
        uint64_t _status;
        //timestamps?

};

class Directory : public Node {

    public:
        Directory(const Ref &ref, std::shared_ptr<Object_Store> backend);

    private:
        std::shared_ptr<Object_Store> _backend;
        Ref _ref;

        std::map<std::string, std::shared_ptr<Node>> _contents;

};

class File : public Node {

    public:
        File(const Ref &ref, const uint32_t &uid, const uint32_t &gid, const uint64_t &status);

    private:
        Ref _ref;

};

class Symlink : public Node {

    public:
        Symlink(const std::string &target, const uint32_t &uid, const uint32_t &gid, const uint64_t &status);

    private:
        std::string _target;

};

#endif
