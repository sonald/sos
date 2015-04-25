#include <unistd.h>
#include <sprintf.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define NAME_MAX 64

static const char* prompt = "sos -> ";

int tty_fd = -1;

static void print(const char* buf)
{
    write(tty_fd, buf, strlen(buf));
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
                write(tty_fd, buf, len);
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
                write(tty_fd, buf, len);
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
void parseCmdline(char* buf, int len, cmd_args_t* cmd)
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

    auto sz = sprintf(log, 1023, "{%s, %d, argv[0]: %s}\n", cmd->cmd, cmd->argc,
        cmd->argc > 0 ? cmd->argv[0]: "NULL");
    log[sz] = 0;
    write(tty_fd, log, sz);
}

static char cmd_buf[64] = "";

#ifdef __cplusplus
extern "C"
#endif
void _start()
{
    pid_t pid = fork();
    if (pid == 0) {
        tty_fd = open("/dev/tty1", O_RDONLY, 0);

        while (tty_fd >= 0) {
            print(prompt);
            int len = read(tty_fd, cmd_buf, sizeof cmd_buf - 1);
            if (len > 0) {
                cmd_buf[len] = 0;
                cmd_args_t args;
                parseCmdline(cmd_buf, len, &args);
                if (strcmp(args.cmd, "ls") == 0) {
                    Ls(args.argc == 0 ? "/": args.argv[0]).execute();
                }
            }
        }

        if (tty_fd >= 0) close(tty_fd);
        // exec(cmd_name, 0, 0);

    } else {
        pid = getpid();
        for(;;) {
            pid_t cpid = wait();

            int len = sprintf(cmd_buf, sizeof cmd_buf - 1, "[A%d child %d exit]  ", pid, cpid);
            write(1, cmd_buf, len);
        }
    }
}
