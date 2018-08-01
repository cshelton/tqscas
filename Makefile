# to make work, you'll need the Fedora 27 singularity image referenced
# below, which is too big for github.  Contact cshelton to get it.
SINGIMG = feddev27.simg
#CXX = singularity exec $(SINGIMG) g++
#CC = singularity exec $(SINGIMG) gcc
#LD = singularity exec $(SINGIMG) ld
CXXFLAGS = -ggdb --std=c++2a
#CXXFLAGS = -O3 --std=c++2a -march=native -mtune=native
#CXXFLAGS = -O3 --std=c++2a
LDFLAGS = 
LDLIBS = # -Lmylib

#%.o: %.cpp
#	$(CC) -c $(CCFLAGS) $< -o $@


