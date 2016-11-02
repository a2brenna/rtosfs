#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include <memory>
#include <rtos/object_store.h>
#include <map>
#include <string>

#include "file.h"

class Directory {

    public:
        Directory(const Ref &ref, std::shared_ptr<Object_Store> backend);

    private:
        std::shared_ptr<Object_Store> _backend;
        Ref _ref;

        std::map<std::string, File> _files;

};

#endif
