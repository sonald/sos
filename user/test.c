#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <lru.h>
#include <stat.h>
#include <sys/wait.h>

#define assert(cond) if (!(cond)) { \
    printf("%s failed", #cond); \
    for(;;); \
}

static void testlru(int argc, char* const argv[])
{
    (void)argc;
    (void)argv;

    LRUCache<int, int> lru(10);
    assert(lru.has(100) == false);
    lru.set(100, 20);
    /*dump(cout, lru);*/

    assert(lru.has(100) != false);
    assert(lru.get(100) == 20);
    for (int i = 0; i < 10; i++) {
        lru.set(i, i);
    }
    assert(lru.has(100) == false);
    lru.set(9, 900); // promote key 9
    lru.set(10, 10);
    assert(lru.has(0) == false); // key 0 became the last
    /*dump(cout, lru);*/

    lru.set(1, 100);
    lru.set(12, 120);
    lru.set(5, 500);
    assert(lru.first() == 500); 
    /*assert(lru.first() == 600); */
    /*dump(cout, lru);*/
}

void testsbrk(int argc, char* const argv[])
{
    (void)argc;
    (void)argv;

    int pid;
    char *a, *b, *c, *lastaddr, *oldbrk, *p;
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
            exit(0);
        }
        *b = 1;
        a = b + 1;
    }

    printf("go fork test\n");
    pid = fork();
    if(pid < 0){
        printf("(char*)sbrk test fork failed\n");
        exit(0);
    }
    c = (char*)sbrk(1);
    c = (char*)sbrk(1);
    if(c != a + 1){
        printf("(char*)sbrk test failed post-fork\n");
        exit(0);
    }
    if(pid == 0)
        exit(0);
    wait(NULL);

    printf("go big test\n");
    // can one grow address space to something big?
#define BIG (2*1024*1024)
    a = (char*)sbrk(0);
    amt = (BIG); //  - (uint32_t)a;
    p = (char*)sbrk(amt);
    if (p != a) { 
        printf("sbrk test failed to grow big address space (p %x, a %x); "
                "enough phys mem?\n", p, a);
        exit(0);
    }
    lastaddr = (char*) (a+BIG-1);
    *lastaddr = 99;

    printf("go dealloc test\n");
    // can one de-allocate?
    a = (char*)sbrk(0);
    c = (char*)sbrk(-4096);
    if(c == (char*)0xffffffff){
        printf("(char*)sbrk could not deallocate\n");
        exit(0);
    }
    c = (char*)sbrk(0);
    if(c != a - 4096){
        printf("(char*)sbrk deallocation produced wrong address, a %x c %x\n", a, c);
        exit(0);
    }

    printf("go realloc test\n");
    // can one re-allocate that page?
    a = (char*)sbrk(0);
    c = (char*)sbrk(4096);
    if(c != a || (char*)sbrk(0) != a + 4096){
        printf("(char*)sbrk re-allocation failed, a %x c %x\n", a, c);
        exit(0);
    }
    /*if(*lastaddr == 99){*/
        /*// should be zero*/
        /*printf("(char*)sbrk de-allocation didn't really deallocate\n");*/
        /*exit(0);*/
    /*}*/

    a = (char*)sbrk(0);
    c = (char*)sbrk(-((char*)sbrk(0) - oldbrk));
    if(c != a){
        printf("(char*)sbrk downsize failed, a %x c %x\n", a, c);
        exit(0);
    }

    if(c == (char*)0xffffffff){
        printf("failed sbrk leaked memory\n");
        exit(0);
    }

    printf("sbrk test OK\n");
}

//can not pass this test, do not has error checking in kernel
void testmem(int argc, char* const argv[])
{
    (void)argc;
    (void)argv;

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
            exit(0);
        }
        free(m1);
        printf("mem ok\n");
        exit(0);
    } else {
        wait(NULL);
    }
}

void testmalloc(int argc, char* const argv[])
{
    (void)argc;
    (void)argv;

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

void testext2(int argc, char* const argv[])
{
    (void)argc;
    (void)argv;

    int sz = 512;
    char* buf = (char*)malloc(sz);
    int fd = open("/mnt/moses-dev/moses-launcher/launchrc.c", O_RDONLY, 0);
    int len = 0;
    while ((len = read(fd, buf, sz)) > 0) {
        write(STDOUT_FILENO, buf, len);
    }
    close(fd);
    free(buf);
}

void testseek(int argc, char* const argv[])
{
    (void)argc;
    (void)argv;

    int sz = 512;
    char* buf = (char*)malloc(sz);
    int fd = open("/mnt/moses-dev/moses-launcher/launchrc.c", O_RDONLY, 0);
    lseek(fd, 100000, SEEK_SET);
    int len = 0;
    while ((len = read(fd, buf, sz)) > 0) {
        write(STDOUT_FILENO, buf, len);
    }
    close(fd);
    free(buf);
}

char* mode_to_str(mode_t m, char* buf)
{
    if (S_ISDIR(m)) buf[0] = 'd';
    if (m & S_IRUSR) buf[1] = 'r';
    if (m & S_IWUSR) buf[2] = 'w';
    if (m & S_IXUSR) buf[3] = 'x';
    if (m & S_IRGRP) buf[4] = 'r';
    if (m & S_IWGRP) buf[5] = 'w';
    if (m & S_IXGRP) buf[6] = 'x';
    if (m & S_IROTH) buf[7] = 'r';
    if (m & S_IWOTH) buf[8] = 'w';
    if (m & S_IXOTH) buf[9] = 'x';
    return buf;
}

void teststat(int argc, char* const argv[])
{
    if (argc <= 2) return; // no filename
    char modes[] = "----------";
    const char* name = argv[2];

    struct stat stbuf;
    if (lstat(name, &stbuf) == 0) {
        printf("mode %s, mtime: %d, uid: %d, gid: %d, sz: %d, "
                "ino: 0x%d\n",
                mode_to_str(stbuf.st_mode, modes), stbuf.st_mtime,
                stbuf.st_uid, stbuf.st_gid,
                stbuf.st_size, stbuf.st_ino);
    }
}

void testpipe(int argc, char* const argv[])
{
    char msg[] = "hello from pipe\n";
    int pfd[2];
    pipe(pfd);

    pid_t pid = fork();
    if (pid == 0) {
        close(pfd[1]);

        sleep(1000);
        close(pfd[0]); // close read, will case SIGPIPE
        printf("child quit\n");
        
    } else if (pid > 0) {
        close(pfd[0]);
        while (1) {
            write(pfd[1], msg, strlen(msg));
            sleep(100);
        }

        // SIGPIPE sent, this won't be called.
        wait(NULL);
        printf("parent quit\n");
    }
}

static void wait1()
{
    pid_t pid = fork();
    if (pid == 0) {
        sleep(2000);
        printf("child %d quit\n", getpid());
        
    } else if (pid > 0) {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            printf("child %d quit\n", getpid());

        } else if (pid2 > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
            printf("wait %d done: %d\n", pid, status);
            sleep(1000);
            waitpid(pid2, &status, 0);
            printf("wait %d done: %d\n", pid2, status);
            printf("parent quit\n");
        }
    }
}

static void wait2()
{
    pid_t pid = fork();
    if (pid == 0) {
        sleep(500);
        printf("child %d quit\n", getpid());
        
    } else if (pid > 0) {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            printf("child %d quit\n", getpid());

        } else if (pid2 > 0) {
            int status = 0;
            while(waitpid(pid, &status, WNOHANG) <= 0) {
                printf("check if %d done\n", pid);
            }
            printf("wait %d done: %d\n", pid, status);
            sleep(100);
            waitpid(pid2, &status, 0);
            printf("wait %d done: %d\n", pid2, status);
            printf("parent quit\n");
        }
    }
}

void testwait(int argc, char* const argv[])
{
    if (argc > 2 && argv[2][0] == '2') {
        wait2();
        return;
    }
    wait1();
}

struct testcase {
    const char* name;
    void (*fn)(int, char*const []);
} tcs[] = {
    {"sbrk", testsbrk},
    {"mem", testmem},
    {"malloc", testmalloc},
    {"lru", testlru},
    {"ext2", testext2},
    {"seek", testseek},
    {"stat", teststat},
    {"pipe", testpipe},
    {"wait", testwait},
};


int main(int argc, char* const argv[])
{
    int N = sizeof(tcs)/sizeof(tcs[0]);

    for (int i = 0; i < N; i++) {
        if (argc == 1 || strcmp(tcs[i].name, argv[1]) == 0) {
            tcs[i].fn(argc,argv);
        }
    }
    return 0;
}


