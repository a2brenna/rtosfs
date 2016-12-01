#include <iostream>
#include <boost/program_options.hpp>
#include <rtos/remote_store.h>
#include <rtos/encode.h>
#include <smpl.h>
#include <smplsocket.h>
#include "file_system.h"

namespace po = boost::program_options;

int main(int argc, char *argv[]){

    std::string RTOSD;
    std::string NODE;
    std::string DIRECTORY;
    std::string FILE;

    po::options_description desc("Options");
    desc.add_options()
        ("uds", po::value<std::string>(&RTOSD), "Unix Domain Socket of rtosd")
        ("node", po::value<std::string>(&NODE), "Base 16 node reference to query")
        ("dir", po::value<std::string>(&NODE), "Base 16 directory reference to query")
        ("file", po::value<std::string>(&FILE), "Base 16 file reference to query")
    ;

    try{
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(...){
        std::cout << desc << std::endl;
        return -1;
    }

    if(RTOSD.size() == 0){
        std::cout << desc << std::endl;
        return -1;
    }

    std::shared_ptr<smpl::Remote_Address> rtosd_address(new smpl::Remote_UDS(RTOSD));
    std::shared_ptr<Object_Store> backend(new Remote_Store(rtosd_address));

    if(NODE.size() > 0){
        std::vector<Inode> inodes;
        {
            const std::string encoded = base16_decode(NODE);
            const Ref node_ref(encoded.c_str(), 32);
            const std::string raw = backend->fetch(node_ref).data();
            assert(raw.size() % (sizeof(Inode)) == 0);
            inodes.resize(raw.size() / sizeof(Inode));
            std::memcpy(&inodes[0], raw.c_str(), raw.size());
        }

        for(size_t i = 0; i < inodes.size(); i++){
            std::cout << "Generation " << i << std::endl;
            std::cout << inodes[i] << std::endl << std::endl;
        }
    }
    else if(DIRECTORY.size() > 0){
        const Ref dir_ref(DIRECTORY);

    }
    else if(FILE.size() > 0){
        const Ref file_ref(FILE);

    }
    else{
        std::cout << desc << std::endl;
    }

    return 0;
}
