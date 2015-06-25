#include <unistd.h>
#include <sprintf.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

static const char* prompt = "sos # ";

static void print(const char* buf)
{
    write(STDOUT_FILENO, buf, strlen(buf));
}

static bool str_caseequal(const char* s1, const char* s2)
{
    const char* p1 = s1, *p2 = s2;
    while (*p1 && *p2) {
        int c1 = tolower(*p1), c2 = tolower(*p2);
        if (c1 != c2) return false;
        p1++, p2++;
    }
}

#define MAX_NR_ARG 10
#define MAX_NR_TOKEN 32
#define MAX_NR_SYM MAX_NR_TOKEN
#define SYM_LEN 128

#define PATH_LEN 1024

typedef struct cmd_args_s {
    char argv[MAX_NR_ARG][SYM_LEN];
    int argc;
} cmd_args_t;

typedef struct io_redirect_s {
    bool apply;
    char desc[SYM_LEN];
} io_redirect_t;

struct Command {
    virtual int execute(bool dofork = false) = 0;
    virtual void dump() = 0;
};

using command_runner_t = int (*)(const cmd_args_t& args);
using command_descriptor_t = void (*)();
static struct builtin_command {
    const char* name;
    command_runner_t fn; 
    command_descriptor_t desc_fn;
} builtins[] = {
    { 
        "kdump",
        [] (const cmd_args_t& args) -> int {
            kdump();
            return 0;
        },
        [] () {
            print("display basic information for all tasks.\n");
        }
    },
    { 
        "help",
        [] (const cmd_args_t& args) -> int {
            if (args.argc > 1) {
                for (int i = 0; builtins[i].name; i++) {
                    if (str_caseequal(args.argv[1], builtins[i].name)) {
                        builtins[i].desc_fn();
                        return 0;
                    }
                }
            }
            char buf[128];
            snprintf(buf, sizeof buf - 1, "builtin commands: \n");
            print(buf);

            snprintf(buf, sizeof buf - 1, "kdump, help \n");
            print(buf);
            return 0;
        },
        [] () {
            print("show help for all builtin commands. use help "
                    " command to show details. \n");
        }
    },
    {
        NULL, NULL, NULL
    }
};

struct BaseCommand: public Command {
    cmd_args_t args;
    io_redirect_t input, output;

    int execute(bool dofork = false) override {
        auto& av = args.argv;
        for (int i = 0; builtins[i].name; i++) {
            if (str_caseequal(av[0], builtins[i].name)) {
                return builtins[i].fn(args);
            }
        }

        const char bin[] = "/bin";
        bool found = false;
        char pathname[PATH_LEN];

        int fd = open(bin, O_RDONLY, 0);
        if (fd >= 0) {
            struct dirent dire;
            while (readdir(fd, &dire, 1) > 0) {
                if (str_caseequal(av[0], dire.d_name)) {
                    snprintf(pathname, PATH_LEN-1, "%s/%s", bin, av[0]);
                    found = true;
                    break;
                }
            }
            close(fd);
        }

        if (found) {
            run(pathname, dofork);
        } else {
            print("can not found executable\n");
        }

        return 0;
    }

    void run(const char* path, bool dofork) {
        int pid = 0;
        if (!dofork || (pid = fork()) >= 0) {
            if (pid > 0) {
                wait();
            } else if (pid == 0) {
                //TODO: io redirection
                int argc = args.argc;
                char* argv[argc+1];
                for (int i = 0; i < argc; i++) {
                    argv[i] = &args.argv[i][0];
                }
                argv[argc] = NULL;
                execve(path, argv, 0);
            }
        } else {
            print("fork error\n");
        }
    }

    void dump() override {
        print("(");
        for (int i = 0; i < args.argc; i++) {
            print(args.argv[i]);
            print(" ");
        }

        if (input.apply) { print("(<"); print(input.desc); print(")"); }
        if (output.apply) { print("(>"); print(output.desc); print(")"); }
        print(")");
    }
};

struct PipeCommand: public Command {
    Command *lhs {NULL}, *rhs {NULL};
    int execute(bool dofork = false) override {
        int pid = 0;
        if (!dofork || (pid = fork()) >= 0) {
            if (pid > 0) {
                wait();

            } else if (pid == 0) {
                int fd[2];
                pipe(fd);

                pid_t pid = fork();
                if (pid == 0) {
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[0]);
                    lhs->execute(false);

                } else if (pid > 0) {
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[1]);
                    rhs->execute(false);
                }
            }
        }

        return 0;
    }

    void dump() override {
        print("(");
        lhs->dump();
        print(" ");
        rhs->dump();
        print(")");
    }
};

enum TokenType {
    T_PIPE,
    T_REDIRECT_INPUT,  // < 
    T_REDIRECT_OUTPUT, // >
    T_STR_LITERAL,
    T_SYMBOL, // all non-white sequence is considered a symbol now
    T_EOF
};

typedef struct token_s {
    TokenType type;
    char* val; // for symbol or literal
} token_t;

//static storages, cause no malloc provided yet
static PipeCommand s_pipes[10];
static int s_nr_pipe = 0;
static BaseCommand s_basecmds[MAX_NR_SYM];
static int s_nr_basecmd = 0;

// symbol and literal values
static char symbols[MAX_NR_SYM*SYM_LEN];
static int sym_mark = 0;

static token_t tokens[MAX_NR_TOKEN];
static int nr_token = 0;
//~

static token_t* peek(char* buf)
{
    if (nr_token == 0) return NULL;
    if (nr_token >= MAX_NR_TOKEN) return NULL;
    return &tokens[nr_token-1];
}

static char* _p = NULL;
static token_t* next(char* buf)
{
    if (nr_token == 0) _p = buf;
    else if (nr_token >= MAX_NR_TOKEN) return NULL;
    while (*_p && isspace(*_p)) _p++;

    token_t* tk = &tokens[nr_token++];
    if (*_p == 0) {
        tk->type = T_EOF;
    } else if (*_p == '>') {
        tk->type = T_REDIRECT_OUTPUT;
        _p++;
    } else if (*_p == '<') {
        tk->type = T_REDIRECT_INPUT;
        _p++;
    } else if (*_p == '|') {
        tk->type = T_PIPE;
        _p++;
    } else if (*_p == '"') {
        tk->type = T_STR_LITERAL;
        tk->val = &symbols[sym_mark];
        _p++;
        while (*_p && *_p != '"') {
            symbols[sym_mark++] = *_p++;
        }
        _p++; // eat right \"
        symbols[sym_mark++] = 0;
    } else {
        tk->type = T_SYMBOL;
        tk->val = &symbols[sym_mark];
        while (*_p && !isspace(*_p)) {
            symbols[sym_mark++] = *_p++;
        }
        symbols[sym_mark++] = 0;
    }
    return tk;
}

static bool expect(char* buf, TokenType tt)
{
    auto* tk = peek(buf);
    if (tk && tk->type != tt) {
        print("parsing error\n");
        return false;
    }
    return true;
}


static Command* parse_base_cmd(char* buf)
{
    BaseCommand* base = &s_basecmds[s_nr_basecmd++];
    cmd_args_t& as = base->args;
    memset(&as, 0, sizeof as);
    as.argc = 0;

    /*print("parse_base_cmd\n");*/
    while (as.argc < MAX_NR_ARG) {
        token_t* tk = next(buf);
        if (!tk || (tk->type == T_EOF || tk->type == T_PIPE)) break;
        switch(tk->type) {
            case T_REDIRECT_INPUT: 
                base->input.apply = true;
                tk = next(buf);
                if (!tk || tk->type != T_SYMBOL) return NULL;
                memcpy(base->input.desc, tk->val, SYM_LEN);
                base->input.desc[SYM_LEN-1] = 0;
                break;

            case T_REDIRECT_OUTPUT: 
                base->output.apply = true;
                tk = next(buf);
                if (!tk || tk->type != T_SYMBOL) return NULL;
                memcpy(base->output.desc, tk->val, SYM_LEN);
                base->output.desc[SYM_LEN-1] = 0;
                break;

            case T_STR_LITERAL: 
            case T_SYMBOL: 
                memcpy(as.argv[as.argc], tk->val, SYM_LEN);
                as.argv[as.argc++][SYM_LEN-1] = 0;
                break;

            default: return NULL; // should not go to ehre
        }
    }

    return base;
}

static Command* parse_pipe(char* buf)
{
    Command* lhs = NULL, *rhs = NULL;

    /*print("parse_pipe\n");*/
    lhs = parse_base_cmd(buf);
    token_t* tk = peek(buf);
    while (tk && tk->type != T_EOF) {
        if (expect(buf, T_PIPE)) {
            /*print("parse_pipe rhs\n");*/
            rhs = parse_base_cmd(buf);
            PipeCommand* pipe = &s_pipes[s_nr_pipe++];
            pipe->lhs = lhs, pipe->rhs = rhs;
            lhs = pipe;

            tk = peek(buf);

        } else {
            return NULL;
        }
    }

    return lhs;
}

static void dump_tokens(char* buf)
{
    token_t* tk = NULL;
    bool quit = false;
    while ((tk = next(buf)) && !quit) {
        switch(tk->type) {
            case T_PIPE: print("| "); break;
            case T_REDIRECT_INPUT: print("< "); break;
            case T_REDIRECT_OUTPUT: print("> "); break;
            case T_STR_LITERAL: print("\'"); print(tk->val); print("\' "); break;
            case T_SYMBOL: print("{"); print(tk->val); print("} "); break;
            case T_EOF: print("{EOF}\n"); quit = true; break;
        }
    }
}

static void reset_parser()
{
    nr_token = 0;
    sym_mark = 0;
    s_nr_pipe = 0;
    s_nr_basecmd = 0;
}

static Command* parse_cmdline(char* buf)
{
    /*if (nr_token > 0) reset_parser();*/
    /*dump_tokens(buf);*/

    if (nr_token > 0) reset_parser();
    return parse_pipe(buf);
}

static char cmd_buf[1024] = "";
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
                auto* cmd = parse_cmdline(cmd_buf);
                if (cmd) {
                    cmd->dump();
                    print("\n");
                    cmd->execute(true);
                }
            }
        }

    } else {
        for(;;) {
            wait();
        }
    }

    return 0;
}
