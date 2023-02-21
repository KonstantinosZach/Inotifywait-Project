#include "worker.h"

//killed by the manager
static void act_term(int signo){
    //exit closes the fds in use
    _exit(EXIT_SUCCESS);
}

//workers after receiving sigint wait to be killed by the manager
static void act_end(int signo){
    pause();
}

Worker::Worker(){ std::cout << "creating worker " << getpid() << std::endl; }

Worker::~Worker(){ std::cout << "deleting worker" << std::endl; }

void Worker::work(int id, std::string path){

    //we set up the singals
    //we dont care to block the others signals inside the handlers
    static struct sigaction end;
    end.sa_handler = act_end;
    end.sa_flags = SA_RESTART;
    if(sigaction(SIGINT, &end, NULL) == -1){
        perror("sigint sigaction");
        _exit(EXIT_FAILURE);
    }

    static struct sigaction term;
    term.sa_handler = act_term;
    term.sa_flags = SA_RESTART;
    if(sigaction(SIGTERM, &term, NULL) == -1){
        perror("sigterm sigaction");
        _exit(EXIT_FAILURE);
    }

    pid_t pid = getpid();
    int readfd;
    while(1){
        //opens pipe and reads the filename that manager sent
        std::string fifo_write = FIFO_WRITE + std::to_string(id);
        std::cout << "worker " << pid << " will open fifo: " << fifo_write << std::endl;

        if((readfd = open(fifo_write.c_str(), O_RDONLY)) < 0){
            perror("client: can't open write fifo \n");
            _exit(EXIT_FAILURE);
        }
        
        int read_check;
        char read_char[1] = {};
        std::string read_buffer = "";
        
        //we read byte-byte until we read all the message
        while((read_check = read(readfd, read_char, 1) > 0)){
            read_buffer.append(read_char);
            memset(read_char, 0, sizeof(read_char));
        }

        if(read_check == -1){
            perror("client: filename read error on file");
            _exit(EXIT_FAILURE);
        }

        //we close the pipe
        if(close(readfd) == -1){
            perror("client: can't close read fifo");
            _exit(EXIT_FAILURE);
        }

        std::cout << "worker " << pid << " read: " << read_buffer << std::endl;

        /*------------------------------------------------------------------*/

        //we get the path of the file that we need to read
        std::string tmp_path;
        if(path.compare("./") == 0)
            tmp_path = read_buffer;
        else
            tmp_path = path + "/" + read_buffer;
        
        std::string read_line = "";
        std::cout << "worker " << pid << " will open the file: " << tmp_path << std::endl;
        
        if((readfd = open(tmp_path.c_str(), O_RDONLY)) < 0){
            perror("worker: can't open the file \n");
            _exit(EXIT_FAILURE);
        }
        
        std::map<std::string,int> sites; //a map that stores the sites found with their count of a occurrences

        while((read_check = read(readfd, read_char, 1)) >= 0 ){
            //until encounter new line or EOF and adding each char to a long string
            //then we can edit it
            if(!strcmp(read_char,"\n") || read_check == 0){
                // Find first occurrence of "http://"
                size_t found = read_line.find("http://");
                while(found != std::string::npos){
                    
                    //if found erase it
                    std::string site = read_line.substr(found);
                    site = site.erase(0,7);

                    //we take the string until the fist white space
                    //if there is not a white space until new line
                    size_t found_tmp = site.find(" ");
                    if(found_tmp != std::string::npos){
                        site = site.substr(0, found_tmp);
                    }

                    //if there is www. in the string we erase it
                    found_tmp = site.find("www.");
                    if(found_tmp != std::string::npos){
                        site = site.erase(found_tmp,4);
                    }

                    //we store only the address
                    found_tmp = site.find("/");
                    if(found_tmp != std::string::npos){
                        site = site.substr(0,found_tmp);
                    }

                    //the site we found we put it on the map
                    std::map<std::string, int>::iterator it;
                    it = sites.find(site);

                    if(it != sites.end())
                        it->second ++;
                    else
                        sites.insert({site, 1});

                    site.clear();
                    //we search for the next occurrence of http://
                    found = read_line.find("http://", found+1);               
                }
                read_line.clear();
            }
            else{
                read_line.append(read_char);
            }
            memset(read_char, 0, sizeof(read_char));

            //if we encountered EOF we exit the while
            if(read_check == 0)
                break;
        }

        if(read_check == -1){
            perror("client: filename read error on file");
            _exit(EXIT_FAILURE);
        }

        if(close(readfd) == -1){
            perror("client: can't close the file");
            _exit(EXIT_FAILURE);
        }   
        
        tmp_path.clear();
        /*----------------------------------------------------------------*/

        //create the file to store the output
        size_t found = read_buffer.find(".");
        int writefd;
        if(found != std::string::npos){
            //the name of the new file must match the name of the given file
            std::string goal = read_buffer.substr(0,found);
            goal = WORKER_OUTPUT + goal + ".out";
            std::cout << "File to create " << goal << std::endl;
        
            if((writefd = open(goal.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){
                perror("worker: can't open the file \n");
                _exit(EXIT_FAILURE);
            }
            
            //we copy the map with the sites to the file that we created
            std::map<std::string,int>::iterator it;
            for(it = sites.begin(); it != sites.end(); ++it ){
                std::string line = it->first + " " + std::to_string(it->second) + "\n";
                int size = strlen(line.c_str());

                if (write(writefd, line.c_str(), size) != size){
                    perror("worker: store on file write error");
                    _exit(EXIT_FAILURE);
                }
            }

            //we close the pipe
            if(close(writefd) == -1){
                perror("worker: can't close file");
                _exit(EXIT_FAILURE);
            }
        }
        /*----------------------------------------------------------------*/

        sites.clear();
        std::cout << "worker " << pid << " will stop " << std::endl;

        //signal to stop
        if(kill(pid,SIGSTOP) == -1){
            perror("unable to stop worker");
            exit(EXIT_FAILURE);
        }

        //we got signal by the parent to continue
        std::cout << "worker " << pid << " will continue " << std::endl;

    }
}