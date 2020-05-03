#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define R 0
#define W 1



int 
main() {
    int total = 35;
    int arr[36], cnt = 0, i;
    int fd[2];

    for(i = 2; i <= total; i++) {
        arr[cnt++] = i;

    }

    while(cnt > 0) {
        if( (i =pipe(fd)) < 0) {
            printf("pipe failed %d\n", i);
        }
        int pid = fork();

        if(pid < 0) {
            printf("fork failed %d\n", pid);
        }
        if(pid == 0) {
            int prime, this_prime = 0;
            close(fd[W]);
            cnt = -1;
            while(read(fd[R], &prime, sizeof(prime)) != 0) {
                if(cnt == -1) {
                    this_prime = prime;
                    cnt = 0;
                    printf("prime: %d\n", this_prime);
                } else {
                    if(prime % this_prime != 0) arr[cnt++] = prime;
                }

            }
            close(fd[R]);
        } else {
            close(fd[R]);
            for(int i=0; i < cnt; i++) {
                write(fd[W], (arr + i), sizeof(arr[i]));
            }
            printf("cnt %d\n", cnt);
            close(fd[W]);
            wait(0);
            break;
        }
    } 

    exit(0);

} 


/*
int primeproc(int fd) {
    int i, id, sub_fd[2], r, this_prime;
top:
    if((r = read(fd, &this_prime, sizeof(this_prime))) != sizeof(this_prime)) {
        panic("primeproc could not read initial prime: %d, %e", r, r >=0 ? 0 : r);
    }

    printf("prime: %d\n", this_prime);

    // fork a right neighbour to continue the chain
    if((i = pipe(sub_fd)) < 0) {
        panic("pipe: %e", i);
    }
    if((id == fork()) <0) {
        panic("fork: %e", id);
    }
    
    if(id == 0) {
        close(fd);
        close(sub_fd[W]);
        fd = sub_fd[R];
        goto top;
    } 

    close(sub_fd[R]);

    for()
}


int 
main() {
    int i, id, fd[2], r;

    if( i = pipe(fd) < 0) {
        panic("pipe: %e", i);
    }   

    // fork the first process in the chain
    if((id = fork()) < 0 ) {
        panic("fork: %e", id);
    } 

    if(id == 0) {
        close(fd[W]);
        primeproc(fd[R]);
    }

    close(fd[R]);
    for(i = 2; i < 35; i++) {
        if(( r = write(fd[W], &i, 4)) != 4)
            panic("generator write: %d, %e", r, r >=0 ? 0 : r);
    }
}
*/