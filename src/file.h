#ifndef __FILE_H__
#define __FILE_H__

#include <rtos/types.h>
#include <string>

class File {

    private:
        Ref _ref;
        //user_id
        //group_id
        //permissions

};

class Symlink {

    private:
        std::string _target;

};

#endif
