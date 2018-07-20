# to make work, you'll need the Fedora 27 singularity image referenced
# below, which is too big for github.  Contact cshelton to get it.
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


