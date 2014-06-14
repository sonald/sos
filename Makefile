CROSS_PATH=/Users/sonald/crossgcc/bin
CXX=$(CROSS_PATH)/i686-elf-g++
CXXFLAGS=-std=c++11 -I./include -ffreestanding  -O2 -Wall -Wextra -fno-exceptions -fno-rtti -g -DDEBUG

OBJS_DIR=objs

crtbegin_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
crtend_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

# order is important, boot.o must be first here
kernel_srcs=boot.s main.cc common.cc cxx_rt.cc irq_stubs.s gdt.cc isr.cc \
	timer.cc mm.cc

kernel_objs := $(patsubst %.cc, $(OBJS_DIR)/%.o, $(kernel_srcs))
kernel_objs := $(patsubst %.s, $(OBJS_DIR)/%.o, $(kernel_objs))

objs := $(OBJS_DIR)/crti.o $(crtbegin_o) $(kernel_objs) $(crtend_o) $(OBJS_DIR)/crtn.o

DEPFILES := $(patsubst %.cc, %.d, $(kernel_srcs))
DEPFILES := $(patsubst %.s, %.d, $(DEPFILES))

-include $(DEPFILES)

all: run

debug: kernel
	qemu-system-i386 -kernel kernel -m 32 -s -S

run: kernel
	qemu-system-i386 -kernel kernel -m 32 

kernel: $(objs) kernel.ld
	$(CXX) -T kernel.ld -O2 -nostdlib -o $@ $^ -lgcc

$(OBJS_DIR)/%.o: %.cc Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<
			
$(OBJS_DIR)/%.o: %.s
	@mkdir -p $(@D)
	nasm -f elf32 -o $@ $<

.PHONY: clean

clean:
	-rm $(OBJS_DIR)/*.o 
	-rm kernel
