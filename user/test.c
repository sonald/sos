#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <printf.h>
#include <stdlib.h>

void sbrktest(void)
{
    int fds[2], pid, pids[10], ppid;
    char *a, *b, *c, *lastaddr, *oldbrk, *p, scratch;
    uint32_t amt;

    printf("sbrk test\n");
    oldbrk = (char*)sbrk(0);

    // can one (char*)sbrk() less than a page?
    a = (char*)sbrk(0);
    int i;
    for(i = 0; i < 5000; i++){ 
        b = (char*)sbrk(1);
        if(b != a){
            printf("(char*)sbrk test failed %d %x %x\n", i, a, b);
            exit();
        }
        *b = 1;
        a = b + 1;
    }

    printf("go fork test\n");
    pid = fork();
    if(pid < 0){
        printf("(char*)sbrk test fork failed\n");
        exit();
    }
    c = (char*)sbrk(1);
    c = (char*)sbrk(1);
    if(c != a + 1){
        printf("(char*)sbrk test failed post-fork\n");
        exit();
    }
    if(pid == 0)
        exit();
    wait();

    printf("go big test\n");
    // can one grow address space to something big?
#define BIG (2*1024*1024)
    a = (char*)sbrk(0);
    amt = (BIG); //  - (uint32_t)a;
    p = (char*)sbrk(amt);
    if (p != a) { 
        printf("sbrk test failed to grow big address space (p %x, a %x); "
                "enough phys mem?\n", p, a);
        exit();
    }
    lastaddr = (char*) (a+BIG-1);
    *lastaddr = 99;

    printf("go dealloc test\n");
    // can one de-allocate?
    a = (char*)sbrk(0);
    c = (char*)sbrk(-4096);
    if(c == (char*)0xffffffff){
        printf("(char*)sbrk could not deallocate\n");
        exit();
    }
    c = (char*)sbrk(0);
    if(c != a - 4096){
        printf("(char*)sbrk deallocation produced wrong address, a %x c %x\n", a, c);
        exit();
    }

    printf("go realloc test\n");
    // can one re-allocate that page?
    a = (char*)sbrk(0);
    c = (char*)sbrk(4096);
    if(c != a || (char*)sbrk(0) != a + 4096){
        printf("(char*)sbrk re-allocation failed, a %x c %x\n", a, c);
        exit();
    }
    /*if(*lastaddr == 99){*/
        /*// should be zero*/
        /*printf("(char*)sbrk de-allocation didn't really deallocate\n");*/
        /*exit();*/
    /*}*/

    a = (char*)sbrk(0);
    c = (char*)sbrk(-((char*)sbrk(0) - oldbrk));
    if(c != a){
        printf("(char*)sbrk downsize failed, a %x c %x\n", a, c);
        exit();
    }

    if(c == (char*)0xffffffff){
        printf("failed sbrk leaked memory\n");
        exit();
    }

    printf("sbrk test OK\n");
}

//can not pass this test, do not has error checking in kernel
void mem(void)
{
    void *m1, *m2;
    int pid, ppid;

    printf("mem test\n");
    ppid = getpid();
    if((pid = fork()) == 0){
        int total = 0x800000;
        m1 = 0;
        while(total > 0 && (m2 = malloc(50001)) != 0){
            printf("m2 = %x, m1 = %x\n", m2, m1);
            *(char**)m2 = (char*)m1;
            m1 = m2;
            total -= 50001;
        }

        if (!m2) {
            printf("failed after %x\n", m1);
        }

        while(m1){
            m2 = *(char**)m1;
            free(m1);
            m1 = m2;
        }
        m1 = malloc(1024*20);
        if(m1 == 0){
            printf("couldn't allocate mem?!!\n");
            /*kill(ppid);*/
            exit();
        }
        free(m1);
        printf("mem ok\n");
        exit();
    } else {
        wait();
    }
}

void testmalloc()
{
    const int PGSIZE = 4096;
    void* p1 = malloc(0x80-3);
    void* p2 = malloc(0x100);
    void* p3 = malloc(0x400);
    // kprintf("p1 = 0x%x, p2 = 0x%x, p3 = 0x%x\n", p1, p2, p3);

    void* p4 = malloc(PGSIZE-1);
    void* p5 = malloc(PGSIZE);
    // kprintf("p4 = 0x%x\n", p4);
    free(p3);
    free(p2);
    free(p4);
    free(p1);

    p1 = malloc(PGSIZE*3);
    free(p5);
    free(p1);

    {
        malloc(PGSIZE);
        malloc(3000);
        void* pg2 = malloc(PGSIZE+3000);
        malloc(PGSIZE);
        malloc(PGSIZE*2);
        free(pg2);
        malloc(PGSIZE);
    }
}

int main(int argc, char* const argv[])
{
    sbrktest();
    mem();
    testmalloc();
    return 0;
}


