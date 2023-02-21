#include "manager.h"
#include "worker.h"
#include "errors.h"


static std::map<pid_t,int> pairs;   //we store all the worker pids with their worker ids
static std::queue<pid_t> workers;   //the queue for the available workers

static pid_t inotifywait_pid;

//informs that there are available workers to be collected
static bool flag = false;
static void act_chld(int signo){
    flag = true;
}

//does the cleanup with the workers after receiving SIGINT
static void act_end(int signo){

    pid_t child_pid;
    int status;
    bool inotifywait_flag = false;
    std::map<pid_t,int> dummy = pairs;  //we dont want to lose the pids of the original map

    //collect all the workers that are stopped but not inside the queue
    while((child_pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
        if(child_pid == -1){
            perror("stopped child waitpid");
            exit(EXIT_FAILURE);
        }

        if(child_pid == inotifywait_pid){
            inotifywait_flag = true;
        }
        else{
            dummy.erase(child_pid);
            if(kill(child_pid, SIGKILL) == -1){
                perror("unable to kill worker");
                exit(EXIT_FAILURE);
            }
            if(waitpid(child_pid, &status, 0) == -1){
                perror("terminated child waitpid");
                exit(EXIT_FAILURE);
            }
        }
    }

    //collect all the workers that are stopped and inside the queue
    while(!workers.empty()){
        child_pid = workers.front();
        dummy.erase(child_pid);
        workers.pop();

        if(kill(child_pid, SIGKILL) == -1){
            perror("unable to kill worker");
            exit(EXIT_FAILURE);
        }
        if(waitpid(child_pid, &status, 0) == -1){
            perror("terminated child waitpid");
            exit(EXIT_FAILURE);
        }
    }

    //kills all the workers that are still on duty and then collect them
    std::map<pid_t,int>::iterator it;
    for(it = dummy.begin(); it != dummy.end(); ++it){
        if(kill(it->first,SIGTERM) == -1){
            perror("unable to kill worker");
            exit(EXIT_FAILURE);
        }
        if(waitpid(it->first, &status, 0) == -1){
            perror("terminated child waitpid");
            exit(EXIT_FAILURE);
        }
    }
    dummy.clear();

    //collects the exec of inotifywait if it didnt already
    if(!inotifywait_flag){
        if(waitpid(inotifywait_pid, &status, 0) == -1){
            perror("inotifywait waitpid");
            exit(EXIT_FAILURE);
        }
    }
}

//prints the available workers
static void printQueue(std::queue<pid_t> queue){
    std::queue<pid_t> temp = queue;
    std::cout << "Available workers queue" << std::endl;
	while (!temp.empty()){
		std::cout << temp.front() << std::endl;
		temp.pop();
	}
	std::cout << std::endl;
}

Manager::Manager(){ std::cout << "creating manager" << std::endl; }

Manager::~Manager(){ std::cout << "deleting manager" << std::endl; }

void Manager::send_message(std::string fifo_name, char* buffer){
    //we get the file name
    char* new_file;
    new_file = strtok(buffer, " ");

    for(int i=0; i<2; i++)
        new_file = strtok(NULL, " ");
    
    //we send the message to the worker
    std::cout << "Manager will send the file " << new_file  
    << " to " << fifo_name.c_str() << std::endl;

    //open the named pipe
    if((writefd = open(fifo_name.c_str(), O_WRONLY)) < 0){
        perror("manager: can't open write fifo \n");
        exit(EXIT_FAILURE);
    }

    //we write to the pipe
    int size = strlen(new_file);
    if(write(writefd, new_file, size) == -1){
        perror("manager: filename write error");
        exit(EXIT_FAILURE);
    }

    //we close the pipe
    if(close(writefd) == -1){
        perror("manager: can't close write fifo");
        exit(EXIT_FAILURE);
    }

    std::cout << "Manager sent the file " << new_file  << std::endl;
}

void Manager::inotify_wait(std::string dir_path){

    //we declare the signals that manager need to handle
    static struct sigaction chld;
    chld.sa_handler = act_chld;
    chld.sa_flags = SA_RESTART;
    if(sigfillset(&(chld.sa_mask)) == -1){
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGCHLD, &chld, NULL) == -1){
        perror("sigchld sigaction");
        exit(EXIT_FAILURE);
    }

    static struct sigaction end;
    end.sa_handler = act_end;
    end.sa_flags=SA_RESTART;
    if(sigfillset(&(end.sa_mask)) == -1){
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }

    if(sigaction(SIGINT, &end, NULL) == -1){
        perror("sigint sigaction");
        exit(EXIT_FAILURE);
    }

    char* dir_path_str = strdup(dir_path.data());

    char* argument_list[] = {
        (char*)".", dir_path_str, (char*)"-m", (char*)"-e",
        (char*)"create", (char*)"-e", (char*)"moved_to", (char *)NULL
        };
    
    std::cout << "creating pipe connection between manager and listener..." << std::endl;
    int pipefd[2];
    if(pipe(pipefd) == -1){
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }
    
    pid_t child_pid;
    if((child_pid = fork()) < 0) {
        perror("failed to create process");
        exit(EXIT_FAILURE);
    }
    
    //redirection stdin-stdout
    if(child_pid == 0) {
        if(close(pipefd[0]) == -1){
            perror("unable to close file descriptor");
            exit(EXIT_FAILURE);
        }
        if(dup2(pipefd[1], 1) == -1){
            perror("can't allocate new file descriptor");
            exit(EXIT_FAILURE);
        }
        if(close(pipefd[1]) == -1){
            perror("unable to close file descriptor");
            exit(EXIT_FAILURE);
        }
        if(execvp((char*)Manager::inotifywait_path, argument_list) == -1){
            perror("inotifywait failed");
            exit(EXIT_FAILURE);
        }
    }
    else{
        free(dir_path_str);
        inotifywait_pid = child_pid;
        if(dup2(pipefd[0], 0) == -1){
            perror("can't allocate new file descriptor");
            exit(EXIT_FAILURE);
        }
        if(close(pipefd[1]) == -1){
            perror("unable to close file descriptor");
            exit(EXIT_FAILURE);
        }

        int n_workers = 0;

        char read_char[1] = {};
        int read_check;
        std::string read_line = "";
        //reads one-char at the time until being interrupted by signal or some error
        while((read_check = read(pipefd[0], read_char, 1)) > 0 && errno != EINTR){
            
            //when read new line we will we khow that we got the whole message by the listener
            if(!strcmp(read_char,"\n")){
                pid_t worker_pid;
                printQueue(workers);

                //if the queue is empty we create another worker
                if(workers.empty()){
                    //create named pipe for the worker
                    std::string fifo_write = FIFO_WRITE + std::to_string(n_workers);
                    std::cout << "fifo to create " << fifo_write.c_str() << std::endl;
                    if((mkfifo(fifo_write.c_str(), PERMS) < 0) && (errno != EEXIST)){
                        perror("can't create fifo");
                        exit(EXIT_FAILURE);
                    }

                    if((worker_pid = fork()) < 0) {
                        perror("Failed to create process");
                        exit(EXIT_FAILURE);
                    }

                    if(worker_pid == 0){
                        Worker::work(n_workers, dir_path);
                    }
                    else{
                        pairs.insert({worker_pid, n_workers});
                        Manager::send_message(fifo_write, (char*)read_line.c_str());
                        n_workers ++;
                    }
                }
                //else we wake up a worker and we remove him from the queue
                else{
                    pid_t wake_up = workers.front();
                    std::map<pid_t,int>::iterator it;
                    it = pairs.find(wake_up);
                    workers.pop();

                    std::cout << "Worker  " << wake_up << " with id " << it->second << " will be used" << std::endl;
                    if(kill(wake_up,SIGCONT) == -1){
                        perror("unable to wake up worker");
                        exit(EXIT_FAILURE);
                    }

                    std::string fifo_write = FIFO_WRITE + std::to_string(it->second);
                    Manager::send_message(fifo_write,(char*)read_line.c_str());
                }
                read_line.clear();
            }
            else{
                read_line.append(read_char);
            }
            memset(read_char, 0, sizeof(read_char));

            //collects the workers that have finished their jobs
            pid_t worker_pid;
            int status;
            while(flag){
                while((worker_pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
                    if(worker_pid == -1){
                        perror("stopped child waitpid");
                        exit(EXIT_FAILURE);
                    }
                    //we add him in the queue
                    std::cout << "Worker " << worker_pid << " is collected" << std::endl;
                    workers.push(worker_pid);
                }
                flag = false;
            }
            
        }
        if(read_check == -1){
            perror("manager: filename read error on file");
            exit(EXIT_FAILURE);
        }
    }
    if(close(pipefd[0]) < 0){
        perror("manager: can't close the pipe with listener");
        exit(EXIT_FAILURE);
    }
    
    //finally unlinking the pipes
    std::map<pid_t,int>::iterator it;
    for(it = pairs.begin(); it != pairs.end(); ++it ){
        std::string fifo_write = FIFO_WRITE + std::to_string(it->second);
        std::cout << "fifo to unlink" << fifo_write.c_str() << std::endl;
        if(unlink(fifo_write.c_str()) < 0){
            perror("client: can't unlink");
            exit(EXIT_FAILURE);
        }
        std::cout << "unlinking..." << std::endl;
    }

    //clear the data
    pairs.clear();
    workers = {};
}
