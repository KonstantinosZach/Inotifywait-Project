#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#pragma once

class Errors{

    public:
    static bool dirExists(char* path);

};