GCC=gcc
FLAGS=-Wall

build: server subscriber

server: server.o 
	$(GCC) -o $@ $^ -lm

server.o: server.c
	$(GCC) $(FLAGS) -c $^

subscriber: subscriber.o
	$(GCC) -o $@ $^ -lm

subscriber.o: subscriber.c
	$(GCC) $(FLAGS) -c $^

clean:
	rm -rf server subscriber