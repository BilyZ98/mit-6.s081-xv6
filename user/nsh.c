#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define EXEC 1
#define PIPE 2
#define REDIR 3

#define MAXARGS 10
#define MAXCMD 10

struct cmd {
    int type;
};

struct redircmd {
    int type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
};

struct execcmd{
    int type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
};

struct pipecmd{
    int type;
    struct cmd *left;
    struct cmd *right;
};



//static struct cmd mycmd[MAXCMD];
static struct pipecmd mypcmd[MAXCMD];
static struct execcmd myecmd[MAXCMD];
static struct redircmd myrcmd[MAXCMD];

//static int cmdcount = 0;
static int pcmdcount = 0;
static int ecmdcount = 0;
static int rcmdcount = 0;


char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int fork1(void);
void panic(char*);
struct cmd *parsecmd(char*);

int getcmd(char *buf, int nbuf) {
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0) 
        return -1;
    return 0;
}

void runcmd(struct cmd *cmd){
    int p[2];
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if(cmd == 0) 
        exit(1);

    switch (cmd->type)
    {
    case EXEC:
        ecmd = (struct execcmd*)cmd;
        if(ecmd->argv[0] == 0)
            exit(1);
        exec(ecmd->argv[0], ecmd->argv);
        fprintf(2, "exec %s failed\n", ecmd->argv[0]);
        break;

    case REDIR:
        rcmd = (struct redircmd*)cmd;
        close(rcmd->fd);
        if(open(rcmd->file, rcmd->mode) < 0){
            fprintf(2, "open %s failed\n", rcmd->file);
            exit(1);
        }
        runcmd(rcmd->cmd);
        break;
    
    case PIPE:
        pcmd = (struct pipecmd*)cmd;
        if(pipe(p) < 0)
            panic("pipe");
        if(fork1() == 0) {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->left);
        }
        if(fork1() == 0) {
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->right);
        }
        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
        break;


    default:
        panic("runcmd");
        break;
    }
    exit(0);
}



// get token
// exec 
// maybe redirect output
int main(void) {
    static char buf[100];
    int fd;

    while((fd = open("console", O_RDWR)) >= 0) {
        if(fd >= 3) {
            close(fd);
            break;
        }
    }

    while (getcmd(buf, sizeof(buf)) >= 0) {
        if(fork1() == 0) {
            runcmd(parsecmd(buf));
        }
        wait(0);
    }
    exit(0);
}

void panic(char *s) {
    fprintf(2, "%s\n",s);
    exit(1);
}

int fork1(void) {
    int pid;
    pid = fork();
    if(pid == -1)
        panic("fork");
    return pid;
} 

struct cmd *execcmd(void) {
    struct execcmd *cmd;

    if(ecmdcount >= MAXARGS) 
        panic("execcmd limit hit");
    cmd = &myecmd[ecmdcount++];
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = EXEC;
    return (struct cmd*)cmd;
}

struct cmd* pipecmd(struct cmd *left, struct cmd *right) {
    struct pipecmd *cmd;

    if(pcmdcount >= MAXCMD)
        panic("pipecmd limit hit");
    cmd = &mypcmd[pcmdcount++];
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = PIPE;
    cmd->left = left;
    cmd->right = right;

    return (struct cmd*)cmd;
}

struct cmd *redircmd(struct cmd* subcmd, char *file, char *efile, int mode, int fd) {
    
    if(rcmdcount >= MAXCMD)
        panic("rcmd limit hit");
    struct redircmd *cmd;
    cmd = &myrcmd[rcmdcount++];
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = REDIR;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->efile = efile;
    cmd->mode = mode;
    cmd->fd = fd;
    return (struct cmd*)cmd;
}

int peek(char **ps, char *es, char *toks)
{
    char *s;

    s = *ps;
    while(s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

int gettoken(char **ps, char *es, char **q, char **eq) {
    char *s;
    int ret;

    s = *ps;
    while(s < es && strchr(whitespace, *s))
        s++;
    if(q)
        *q = s;
    ret = *s;
    switch (*s)
    {
    case 0:
        break;
    
    case '|':
    case '<':
        s++;
        break;
    case '>':
        s++;
        if(*s == '>'){
            ret = '+';
            s++;
        }
        break;
    default:
        ret =  'a';
        while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if(eq)
        *eq = s;

    while(s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return ret;
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);


struct cmd *parsecmd(char *s) {
    char *es;
    struct cmd *cmd;

    es = s + strlen(s);
    cmd = parsepipe(&s, es);
    peek(&s, es, "");
    if(s != es) {
        fprintf(2, "leftovers: %s\n", s);
        panic("syntax");
    }
    nulterminate(cmd);
    return cmd;
}



struct cmd* parsepipe(char **ps, char *es) {
    struct cmd *cmd;

    cmd = parseexec(ps, es);
    if(peek(ps, es, "|")) {
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es) {
    int tok;
    char *q, *eq;

    while(peek(ps, es, "<>")) {
        tok = gettoken(ps, es, 0, 0);
        if(gettoken(ps, es, &q, &eq) != 'a')
            panic("missing file for redirection");
        switch (tok)
        {
        case '<':
            cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
            break;

        case '>':
            cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
            break;
        case '+':
            cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
            break;
        
        default:
            break;
        }
    }
    return cmd;
}

struct cmd* parseexec(char **ps, char *es) {
    char *q, *eq;
    int tok, argc;
    struct execcmd *cmd;
    struct cmd *ret;

    ret = execcmd();
    cmd = (struct execcmd*)ret;

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while(!peek(ps, es, "|")){
        if((tok=gettoken(ps, es, &q, &eq)) == 0)
            break;
        if(tok != 'a')
            panic("syntax");
        cmd->argv[argc] = q;
        cmd->eargv[argc] = eq;
        argc++;
        if(argc >= MAXARGS)
            panic("too many args");
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    cmd->eargv[argc] = 0;
    return ret;

}


struct cmd *nulterminate(struct cmd *cmd) {
    int i;
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if(cmd == 0)
        return 0;

    switch (cmd->type)
    {
    case EXEC:
        ecmd = (struct execcmd*)cmd;
        for(i=0; ecmd->argv[i]; i++)
            *ecmd->eargv[i] = 0;
        break;
    
    case REDIR:
        rcmd = (struct redircmd*)cmd;
        nulterminate(rcmd->cmd);
        *rcmd->efile = 0;
        break;

    case PIPE:
        pcmd = (struct pipecmd*)cmd;
        nulterminate(pcmd->left);
        nulterminate(pcmd->right);
        break;

    default:
        break;
    }
    return cmd;
}