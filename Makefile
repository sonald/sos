CROSS_PATH=/Users/sonald/crossgcc/bin
CXX=$(CROSS_PATH)/i686-elf-g++
CC=$(CROSS_PATH)/i686-elf-gcc
CXXFLAGS=-std=c++11 -I./include -ffreestanding  \
		 -O2 -Wall -Wextra -fno-exceptions -fno-rtti -DDEBUG

USER_FLAGS=-std=c++11 -I./include -ffreestanding  \
	   -O2 -Wall -Wextra -fno-exceptions -fno-rtti -DDEBUG

OBJS_DIR=objs

crtbegin_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
crtend_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

# order is important, boot.o must be first here
kernel_srcs=kern/boot.s kern/main.cc kern/common.cc kern/cxx_rt.cc \
			kern/irq_stubs.s kern/gdt.cc kern/isr.cc kern/timer.cc \
			kern/mm.cc kern/vm.cc kern/kb.cc kern/context.s \
			kern/syscall.cc kern/task.cc kern/vfs.cc kern/ramfs.cc \
			kern/ata.cc kern/blkio.cc kern/devices.cc \
			lib/string.cc lib/sprintf.cc 

kernel_objs := $(patsubst %.cc, $(OBJS_DIR)/%.o, $(kernel_srcs))
kernel_objs := $(patsubst %.s, $(OBJS_DIR)/%.o, $(kernel_objs))

kern_objs := $(OBJS_DIR)/kern/crti.o $(crtbegin_o) \
	$(kernel_objs) \
	$(crtend_o) $(OBJS_DIR)/kern/crtn.o

DEPFILES := $(patsubst %.cc, kern_objs/%.d, $(kernel_srcs))
DEPFILES := $(patsubst %.s, kern_objs/%.d, $(DEPFILES))

all: run ramfs_gen

debug: kernel
	qemu-system-i386 -kernel kernel -initrd initramfs.img -m 32 -s -monitor stdio -drive file=hd0.img,format=raw -S

run: kernel echo
	qemu-system-i386 -kernel kernel -initrd initramfs.img -m 32 -s -monitor stdio -drive file=hd0.img,format=raw

kernel: $(kern_objs) kern/kernel.ld
	$(CXX) -T kern/kernel.ld -O2 -nostdlib -o $@ $^ -lgcc

$(OBJS_DIR)/kern/%.o: kern/%.cc Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<
			
$(OBJS_DIR)/kern/%.o: kern/%.s
	@mkdir -p $(@D)
	nasm -f elf32 -o $@ $<

$(OBJS_DIR)/lib/%.o: lib/%.cc Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPFILES)

# tools
ramfs_gen: tools/ramfs_gen.c
	gcc -o $@ $^

# user prog
echo: user/echo.c lib/sprintf.cc user/user.ld 
	@mkdir -p $(@D)
	$(CXX) $(USER_FLAGS) -T user/user.ld -nostdlib -o $@ $^ 
	./ramfs_gen README.md user/echo.c echo

	
.PHONY: clean

clean:
	-rm $(OBJS_DIR)/kern/*.o 
	-rm $(OBJS_DIR)/kern/*.d
	-rm kernel
