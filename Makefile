CFLAGS= -Wall
all: hw3_cri.cpp
	clang++  --std=c++11 $(CFLAGS) hw3_cri.cpp -o hw3.out