# A SIMPY MAKEFILE FOR C++ PROGRAMS

# paths
INCLUDE = ../include
MODULES = ../modules

#compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -Wall  -g -I$(INCLUDE) -std=c++11

# Αρχεία .o
OBJS =  main.o $(MODULES)/manager.o	$(MODULES)/errors.o $(MODULES)/worker.o

# Το εκτελέσιμο πρόγραμμα
EXEC = sniffer 

# Command line arguments
ARGS = -p ../monitoring

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

clean:
	rm -f $(OBJS) $(EXEC) \
	rm -f ../monitoring/*.txt \
	rm -f ../worker_outputs/*.out \
	rm -f *.txt \
	rm -f ../tdl_results

rm:
	rm -d worker_outputs

run: $(EXEC)
	./$(EXEC) $(ARGS)

valgrind:
	valgrind --leak-check=full ./$(EXEC) $(ARGS)