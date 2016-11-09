#include "file_system.h"

File_System::File_System(const std::string &prefix, std::shared_ptr<Object_Store> rtos){
    _prefix = prefix;
    _rtos = rtos;
}
