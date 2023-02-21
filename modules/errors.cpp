#include "errors.h"

bool Errors::dirExists (char* path)
{
    int flags = O_DIRECTORY | O_RDONLY;
    int mode = S_IRUSR | S_IWUSR;
    int fd = open(path, flags, mode);

    if (fd < 0){
        return false;
    }
    else{
        close (fd);
        return true;
    }

}