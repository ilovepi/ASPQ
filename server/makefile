CC=clang++
INCLUDE=~/boost_1_57_0
#SOURCE=src
FLAGS=-std=c++11 -stdlib=libc++ -O2 #-g -fsanitize=address


all:
	$(CC) main.cpp -I $(INCLUDE) $(FLAGS)

aspq.o: aspq.hpp aspq.cpp
	$(CC) aspq.cpp -I $(INCLUDE) $(FLAGS) -o aspq.o


clean:
	rm a.out
