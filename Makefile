CXX=i686-elf-g++
CXXFLAGS=-std=c++11 -I./include -ffreestanding  -O2 -Wall -Wextra -fno-exceptions -fno-rtti -g

crtbegin_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
crtend_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

# order is important, boot.o must be first here
kernel_objs=boot.o main.o common.o runtime.o 

objs=crti.o $(crtbegin_o) $(kernel_objs) $(crtend_o) crtn.o

all: run

run: kernel
	qemu-system-i386 -kernel kernel

kernel: $(objs) kernel.ld
	$(CXX) -T kernel.ld -O2 -nostdlib -o $@ $^ -lgcc

.s.o:
	nasm -f elf32 -o $@ $<

%.o: %.cc %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<
			
.PHONY: clean

clean:
	-rm *.o 
	-rm kernel
