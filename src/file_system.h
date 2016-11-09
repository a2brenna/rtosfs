#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include <memory>
#include <rtos/object_store.h>

class File_System {

    public:
        File_System(const std::string &prefix, std::shared_ptr<Object_Store> rtos);

    private:
        std::string _prefix;
        std::shared_ptr<Object_Store> _rtos;

};

#endif
