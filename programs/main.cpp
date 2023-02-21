#include "manager.h"
#include "errors.h"
#include <string.h>

int main (int argc, char **argv){

    std::string path;
    if(argc == 3){
        if(strcmp(argv[1],"-p"))
            exit(EXIT_FAILURE);

        if(!Errors::dirExists(argv[2])){
            std::cout << "Directory for listener does not exist" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        path.assign(argv[2]);
    }
    else if(argc == 1){
        path.assign("./");
    }
    else{
        exit(EXIT_FAILURE);
    }

    Manager* manager = new Manager();
    manager->inotify_wait(path);
    delete(manager);

    exit(EXIT_SUCCESS);
}