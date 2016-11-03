#ifndef __NODE_H__
#define __NODE_H__

#include <memory>
#include <rtos/object_store.h>
#include <rtos/types.h>
#include <map>
#include <string>

class Node {
    /*
    public:
        user_id
        group_id
        permissions
    */
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

    private:
        Ref _ref;

};

class Symlink : public Node {

    private:
        std::string _target;

};

#endif
