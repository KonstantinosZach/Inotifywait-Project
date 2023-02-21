#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <csignal>
#include <sys/wait.h>
#include <queue>
#include <map>

#define PERMS 0666
#define DIR_PERMS 0777
#define DIR_NAME "../worker_outputs"
#define FIFO_READ "/tmp/fifo_read_"
#define FIFO_WRITE "/tmp/fifo_write_"

#pragma once

class Manager{

    public:
        const char* inotifywait_path = "/usr/bin/inotifywait";
        int writefd;

        Manager();
        ~Manager();
        void inotify_wait(std::string path);
        void send_message(std::string fifo_name, char* buffer);
        char* get_message(const char* fifo_name);
        void add_worker(pid_t worker);
};