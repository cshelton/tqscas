SINGIMG = feddev27.simg
CXX = singularity exec $(SINGIMG) g++
CC = singularity exec $(SINGIMG) gcc
LD = singularity exec $(SINGIMG) ld
CFLAGS = -ggdb
CXXFLAGS = -ggdb --std=c++17
LDFLAGS = 
LDLIBS = # -Lmylib

#%.o: %.cpp
#	$(CC) -c $(CCFLAGS) $< -o $@


