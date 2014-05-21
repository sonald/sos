CXX=i686-elf-g++
CXXFLAGS=-std=c++11 -I./include -ffreestanding  -O2 -Wall -Wextra -fno-exceptions -fno-rtti -g

# order is important
objs=boot.o main.o common.o

all: run

run: kernel

kernel: $(objs) kernel.ld
	$(CXX) -T kernel.ld -O2 -nostdlib -o $@ $^ -lgcc

.s.o:
	nasm -f elf32 -o $@ $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<
			
.PHONY: clean

clean:
	-rm *.o 
	-rm kernel
