#include <boost/program_options.hpp>
#include <string>
#include <iostream>
#include <memory>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <smpl.h>
#include <smplsocket.h>

#include "operations.h"

namespace po = boost::program_options;

static std::unique_ptr<smpl::Channel> rtos;

int main(int argc, char *argv[]){

	std::string RTOSD;
	std::string FS;
    std::string MOUNTPOINT;

    po::options_description desc("Options");
    desc.add_options()
        ("rtosd", po::value<std::string>(&RTOSD), "Unix Domain Socket of rtosd")
        ("fs", po::value<std::string>(&FS), "File System to mount")
        ("mountpoint", po::value<std::string>(&MOUNTPOINT), "Mountpoint to mount File System on")
    ;

    po::positional_options_description pos_desc;
    pos_desc.add("rtosd", 1);
    pos_desc.add("mountpoint", 1);

    try{
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(...){
        std::cout << desc << std::endl;
        return -1;
    }

	if( (FS.size() == 0) || (RTOSD.size() == 0) || (MOUNTPOINT.size() == 0) ){
        std::cout << desc << std::endl;
        return -1;
	}

    std::unique_ptr<smpl::Remote_Address> rtosd_address(new smpl::Remote_UDS(RTOSD));
    rtos = std::unique_ptr<smpl::Channel>(rtosd_address->connect());

    int fargc = 2;
    char* fargv[2];

    fargv[0] = argv[0];

    fargv[1] = (char *)malloc(sizeof(char) * MOUNTPOINT.size() + 1);
    std::strncpy(fargv[1], MOUNTPOINT.c_str(), MOUNTPOINT.size() + 1);

    return fuse_main(fargc, fargv, &rtos_ops, NULL);
}
