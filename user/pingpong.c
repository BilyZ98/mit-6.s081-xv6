#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/*
int main() {
    int parent_fds[2];
    int child_fds[2];
    char buf[1];
    char child_buf_read[1];
    pipe(parent_fds);
    pipe(child_fds);

    int pid;
    pid = fork();

    printf("fork() return %d\n", pid);

    if(pid == 0) {
        read(parent_fds[0], buf, sizeof(buf));
        int child_pid = getpid();
        printf("%d: received ping\n", child_pid);
        printf("received byte is %c\n", buf[0]);
    } else{
        write(parent_fds[1], "h", 1);
    }

    if(pid == 0) {
        write(child_fds[1], buf, sizeof(buf));
    } else {
        int parent_pid = getpid();
        read(child_fds[0], child_buf_read, sizeof(child_buf_read));
        printf("%d: received pong\n", parent_pid);
        printf("received byte is %c\n", child_buf_read[0]);
    }

    exit(0);

}
*/

#define R 0
#define W 1


int 
main() {
    int parent_fd[2], child_fd[2];
    pipe(parent_fd);
    pipe(child_fd);

    if(fork() == 0) {
        close(parent_fd[W]);
        close(child_fd[R]);
        char buffer = ' ';
        read(parent_fd[R], &buffer, 1);
        printf("%d: recevied ping\n", getpid());
        write(child_fd[W], &buffer, 1);
        close(child_fd[W]);
        close(parent_fd[R]);

    } else {
        close(parent_fd[R]);
        close(child_fd[W]);
        char buffer = ' ';
        write(parent_fd[W], &buffer, 1);
        read(child_fd[R], &buffer, 1);
        printf("%d: received pong\n", getpid());
        close(parent_fd[W]);
        close(child_fd[R]);
    }

    exit(0);
    
}