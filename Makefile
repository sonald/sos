CROSS_PATH=/Users/sonald/crossgcc/bin
CXX=$(CROSS_PATH)/i686-elf-g++
CXXFLAGS=-std=c++11 -I./include -ffreestanding  -O2 -Wall -Wextra -fno-exceptions -fno-rtti -g -DDEBUG

OBJS_DIR=objs

crtbegin_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
crtend_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

# order is important, boot.o must be first here
kernel_srcs=kern/boot.s kern/main.cc kern/common.cc kern/cxx_rt.cc \
			kern/irq_stubs.s kern/gdt.cc kern/isr.cc kern/timer.cc \
			kern/mm.cc kern/vm.cc kern/kb.cc kern/context.s \
			kern/syscall.cc kern/task.cc kern/vfs.cc

kernel_objs := $(patsubst %.cc, $(OBJS_DIR)/%.o, $(kernel_srcs))
kernel_objs := $(patsubst %.s, $(OBJS_DIR)/%.o, $(kernel_objs))

kern_objs := $(OBJS_DIR)/kern/crti.o $(crtbegin_o) \
	$(kernel_objs) \
	$(crtend_o) $(OBJS_DIR)/kern/crtn.o

DEPFILES := $(patsubst %.cc, kern_objs/%.d, $(kernel_srcs))
DEPFILES := $(patsubst %.s, kern_objs/%.d, $(DEPFILES))

all: run ramfs_gen

debug: kernel
	qemu-system-i386 -kernel kernel -m 32 -s -S

run: kernel
	qemu-system-i386 -kernel kernel -m 32 -s -monitor stdio

kernel: $(kern_objs) kern/kernel.ld
	$(CXX) -T kern/kernel.ld -O2 -nostdlib -o $@ $^ -lgcc

$(OBJS_DIR)/kern/%.o: kern/%.cc Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<
			
$(OBJS_DIR)/kern/%.o: kern/%.s
	@mkdir -p $(@D)
	nasm -f elf32 -o $@ $<

-include $(DEPFILES)

# tools
ramfs_gen: tools/ramfs_gen.c
	$(CXX) -o $@ $^

# user prog
echo: user/echo.c user/user.ld
	$(CXX) $(CXXFLAGS) -T user/user.ld -O2 -nostdlib -o $@ $^ -lgcc
	
.PHONY: clean

clean:
	-rm $(OBJS_DIR)/kern/*.o 
	-rm $(OBJS_DIR)/kern/*.d
	-rm kernel
