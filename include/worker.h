#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <map>

#define FIFO_READ "/tmp/fifo_read_"
#define FIFO_WRITE "/tmp/fifo_write_"
#define WORKER_OUTPUT "../worker_outputs/"

#pragma once

class Worker{

    public:
        Worker();
        ~Worker();
        static void work(int id, std::string path);
};