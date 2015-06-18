CROSS_PATH = /Users/sonald/crossgcc/bin
CXX = $(CROSS_PATH)/i686-elf-g++
CPP = $(CROSS_PATH)/i686-elf-cpp
CC = $(CROSS_PATH)/i686-elf-gcc
CXXFLAGS = -std=c++11 -I./include -ffreestanding  \
		 -O2 -Wall -Wextra -fno-exceptions -fno-rtti -DDEBUG -fno-strict-aliasing

USER_FLAGS = -std=c++11 -I./include -ffreestanding  \
	   -O2 -Wall -Wextra -fno-exceptions -fno-rtti -DDEBUG

OBJS_DIR = objs

crtbegin_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
crtend_o=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)

kernel_srcs = kern/boot.s kern/core/irq_stubs.s kern/core/context.s \
	$(wildcard kern/*.cc) \
	$(wildcard kern/runtime/*.cc) \
	$(wildcard kern/core/*.cc) \
	$(wildcard kern/drv/*.cc) \
	$(wildcard kern/utils/*.cc) \
	$(wildcard lib/*.cc)

kernel_objs := $(patsubst %.cc, $(OBJS_DIR)/%.o, $(kernel_srcs))
kernel_objs := $(patsubst %.s, $(OBJS_DIR)/%.o, $(kernel_objs))

kern_objs := $(OBJS_DIR)/kern/runtime/crti.o $(crtbegin_o) \
	$(kernel_objs) \
	$(crtend_o) $(OBJS_DIR)/kern/runtime/crtn.o

DEPFILES := $(patsubst %.cc, $(OBJS_DIR)/%.d, $(kernel_srcs))
DEPFILES := $(patsubst %.s, $(OBJS_DIR)/%.d, $(DEPFILES))

ulib = $(wildcard lib/*.cc)

all: run ramfs_gen

-include $(DEPFILES)

$(OBJS_DIR)/kern/%.d: kern/%.cc
	@mkdir -p $(@D)
	$(CPP) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

$(OBJS_DIR)/lib/%.d: lib/%.cc
	@mkdir -p $(@D)
	$(CPP) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

# print makefile variable (for debug purpose)
print-%: ; @echo $* = $($*)

debug: kernel
	qemu-system-i386 -kernel kernel -initrd initramfs.img -m 32 -s -monitor stdio \
	-drive file=hd.img,format=raw -vga vmware

run: kernel hd.img initramfs.img
	qemu-system-i386 -m 32 -s -monitor stdio -drive file=hd.img,format=raw -vga vmware

hd.img: kernel bin/init bin/echo initramfs.img logo.ppm
	hdiutil attach hd.img
	cp kernel /Volumes/SOS
	cp logo.ppm /Volumes/SOS
	cp bin/init /Volumes/SOS
	cp bin/echo /Volumes/SOS/bin
	cp initramfs.img /Volumes/SOS
	#hdiutil detach disk2

kernel: $(kern_objs) kern/kernel.ld
	$(CXX) -T kern/kernel.ld -O2 -nostdlib -o $@ $^ -lgcc

$(OBJS_DIR)/kern/%.o: kern/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJS_DIR)/kern/%.o: kern/%.s
	@mkdir -p $(@D)
	nasm -f elf32 -o $@ $<

$(OBJS_DIR)/lib/%.o: lib/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# tools
ramfs_gen: tools/ramfs_gen.c
	gcc -o $@ $^

# user prog
bin/%: user/%.c user/libc.c $(ulib) user/user.ld
	@mkdir -p $(@D)
	$(CXX) $(USER_FLAGS) -T user/user.ld -nostdlib -o $@ $^

initramfs.img: bin/echo ramfs_gen
	./ramfs_gen README.md user/echo.c bin/echo

.PHONY: clean

clean:
	-rm $(OBJS_DIR)/kern/*.o
	-rm $(OBJS_DIR)/lib/*.o
	-rm $(OBJS_DIR)/kern/core/*.o
	-rm $(OBJS_DIR)/kern/runtime/*.o
	-rm $(OBJS_DIR)/kern/utils/*.o
	-rm $(OBJS_DIR)/kern/drv/*.o
	-rm $(OBJS_DIR)/kern/*.d
	-rm $(OBJS_DIR)/kern/core/*.d
	-rm $(OBJS_DIR)/kern/runtime/*.d
	-rm $(OBJS_DIR)/kern/utils/*.d
	-rm $(OBJS_DIR)/kern/drv/*.d
	-rm $(OBJS_DIR)/lib/*.d
	-rm echo init
	-rm kernel
