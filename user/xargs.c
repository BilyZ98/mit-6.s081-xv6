#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"


int
main(int argc, char*argv[]) {
    if(argc < 2) {
        fprintf(2, "xargs not enough args\n");
        exit(1);
    }

    char *_argv[MAXARG];
    for(int i=1; i < argc; i++) _argv[i - 1] = argv[i];

    char arguments[1000];
    int stat = 1;
    while(stat) {

        int i=0;
        char buf;
        int child_argc = argc - 1;
        int lst_arg = 0;
        while(1) {
            if( (stat = read(0,  &buf, sizeof(buf))) == 0) exit(0);
            if(buf == ' ' || buf == '\n') {
                arguments[i++] = 0;
                _argv[child_argc++] = &arguments[lst_arg];
                lst_arg = i;
            } else {
                arguments[i++] = buf;
            }

            if(buf == '\n') {
                //exec(_argv[0], _argv);
                break;
                //exit(0);                    
            }

            
        }

        _argv[child_argc] = 0;
        int pid = fork();
        if(pid == 0) {
            exec(_argv[0], _argv);
        } else {
            wait(0);
        }
    }

    exit(0);

}



    /*
                if(arguments[i] == ' ') {
                    _argv[argc++] = arguments;
                    i = 0;
                }
                if(arguments[i] == '\n') {
                    exec(_argv[0], _argv + 1);
                    exit(0);
                }
    */


/*

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "Usage: xargs command\n");
    exit(0);
  }

  char* _argv[MAXARG];
  for (int i = 1; i < argc; i++) _argv[i - 1] = argv[i];
  char argument[1000];
  int stat = 1;
  while (stat) {
    int cnt = 0, lst_arg = 0, argv_cnt = argc - 1;
    char ch = 0;
    while (1) {
      stat = read(0, &ch, 1);
      if (stat == 0) exit(0);
      if (ch == ' ' || ch == '\n') {
        argument[cnt++] = 0;
        _argv[argv_cnt++] = &argument[lst_arg];
        lst_arg = cnt;
        if (ch == '\n') break;
        } else argument[cnt++] = ch;
    }
    _argv[argv_cnt] = 0;
    if (fork() == 0) {
      exec(_argv[0], _argv);
    } else {
      wait(0);  
    }
  }

  exit(0);
}

*/