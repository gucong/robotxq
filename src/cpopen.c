#include <unistd.h>
#include <stdio.h>

pid_t cpopen(FILE **fp, char *prog)
{
    int fd1[2], fd2[2];
    pid_t pid;

    if (pipe(fd1) < 0 || pipe(fd2) < 0)
        return -1;    /* open pipe error */

    if ((pid = fork()) < 0)
        return -2;    /* fork error */

    else if (pid > 0) {    /* parent */
        close(fd1[0]);
        close(fd2[1]);
        if (!(fp[0] = fdopen(fd2[0], "r")))    /* form coprocess */
            return -4;    /* fdopen error */
        if (!(fp[1] = fdopen(fd1[1], "w")))    /* to coprocess */
            return -4;    /* fdopen error */
        if (setvbuf(fp[0], NULL, _IONBF, 0) != 0)
            return -3;    /* setvbuf error */
        if (setvbuf(fp[1], NULL, _IONBF, 0) != 0)
            return -3;    /* setvbuf error */
        return pid;
    }

    else {    /* child */
        close(fd1[1]);
        close(fd2[0]);
        if (fd1[0] != STDIN_FILENO) {
            if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO)
                perror("dup2");
            close(fd1[0]);
        }
        if (fd2[1] != STDOUT_FILENO) {
            if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO)
                perror("dup2");
            close(fd2[1]);
        }
        if (execlp("sh", "sh", "-c", prog, NULL) < 0) {
            perror("exec");
        }
    }
    return 0;    /* not reached */
}
