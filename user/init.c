#include <unistd.h>
#include <sprintf.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define NAME_MAX 64

static const char* prompt = "sos # ";

static void print(const char* buf)
{
    write(STDOUT_FILENO, buf, strlen(buf));
}

struct Command {
    char name[NAME_MAX];
    virtual int execute() = 0;
};

struct Ls: public Command {
    Ls(const char* dir): dir{dir} { strcpy(name, "ls"); }
    const char* dir {NULL};

    int execute() override {
        int fd = open(dir, O_RDONLY, 0);
        if (fd >= 0) {
            struct dirent dire;
            while (readdir(fd, &dire, 1) > 0) {
                char buf[64] = "";
                int len = sprintf(buf, sizeof buf - 1, "%s, ", dire.d_name);
                write(STDOUT_FILENO, buf, len);
            }
            close(fd);
        }
        return 0;
    }
};

struct Cat: public Command {
    Cat(const char* filepath): filepath{filepath} { strcpy(name, "cat"); }
    const char* filepath {NULL};

    int execute() override {
        int fd = open(filepath, O_RDONLY, 0);
        if (fd >= 0) {
            char buf[64];
            int len = 0;
            while ((len = read(fd, buf, sizeof buf - 1)) > 0) {
                buf[len] = 0;
                write(STDOUT_FILENO, buf, len);
            }

            close(fd);
        }
        return 0;
    }
};

typedef struct cmd_args_s {
    char cmd[NAME_MAX];
    char argv[6][NAME_MAX];
    int argc;
} cmd_args_t;

char log[512] = "";
void parse_cmdline(char* buf, int len, cmd_args_t* cmd)
{
    char* p = buf, *s = buf;
    bool inarg = true;
    memset((char*)cmd, 0, sizeof *cmd);

    while (*p) {
        if (isspace((int)*p)) {
            if (inarg && (p - s > 0)) {
                auto sz = p - s;
                auto* sp = cmd->cmd[0] ? cmd->argv[cmd->argc++] : cmd->cmd;

                strncpy(sp, s, sz);
                sp[sz] = 0;
            }
            inarg = false;

        } else if (!inarg) {
            s = p;
            inarg = true;
        }
        p++;
    }

    if (inarg && (p - s > 0)) {
        auto sz = p - s;
        auto* sp = cmd->cmd[0] ? cmd->argv[cmd->argc++] : cmd->cmd;
        strncpy(sp, s, sz);
        sp[sz] = 0;
    }
}

static bool str_caseequal(const char* s1, const char* s2)
{
    //kprintf("%s: %s, %s \n", __func__, s1, s2);
    const char* p1 = s1, *p2 = s2;
    while (*p1 && *p2) {
        int c1 = tolower(*p1), c2 = tolower(*p2);
        if (c1 != c2) return false;
        p1++, p2++;
    }

    return *p1 == 0 && *p2 == 0;
}

static void try_external_app(cmd_args_t* args)
{
    const char bin[] = "/bin";
    char buf[64] = "";
    bool found = false;

    int fd = open(bin, O_RDONLY, 0);
    if (fd >= 0) {
        struct dirent dire;
        while (readdir(fd, &dire, 1) > 0) {
            int len = sprintf(buf, sizeof buf - 1, "%s/%s", bin, dire.d_name);
            buf[len] = 0;
            if (str_caseequal(args->cmd, dire.d_name)) {
                print("found cmd\n");
                found = true;
                break;
            }
        }
        close(fd);

        if (found) {
            int pid = fork();
            if (pid > 0) {
                wait();

            } else if (pid == 0) {
                execve(buf, 0, 0);
            } else {
                print("fork error\n");
            }
        }
    }
}

static char cmd_buf[64] = "";

static bool quit = false;

int main()
{
    // init is responsible for opening standard fds
    open("/dev/tty1", O_RDONLY, 0); // stdin
    open("/dev/tty1", O_WRONLY, 0); // stdout
    open("/dev/tty1", O_WRONLY, 0); // stderr

    pid_t pid = fork();
    if (pid == 0) {
        while (!quit) {
            print(prompt);
            int len = read(STDIN_FILENO, cmd_buf, sizeof cmd_buf - 1);
            if (len > 0) {
                cmd_buf[len] = 0;
                cmd_args_t args;
                parse_cmdline(cmd_buf, len, &args);

                if (strcmp(args.cmd, "ls") == 0) {
                    Ls(args.argc == 0 ? "/": args.argv[0]).execute();
                } else if (strcmp(args.cmd, "cat") == 0) {
                    if (args.argc) {
                        Cat(args.argv[0]).execute();
                    }
                }  else {
                    try_external_app(&args);
                }
            }
        }

    } else {
        pid = getpid();
        for(;;) {
            pid_t cpid = wait();

            int len = sprintf(cmd_buf, sizeof cmd_buf - 1, "[A%d child %d exit]  ", pid, cpid);
            write(1, cmd_buf, len);
        }
    }

    return 0;
}
