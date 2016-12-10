#include "debug.h"
#include <string>

const std::string _debug_log_file = "/tmp/rtosfs.log";

std::ofstream _debug_log(){
    std::ofstream _out;
    //_out.open(_debug_log_file, std::ios::app);
    return _out;
}
