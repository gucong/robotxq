#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "interface.h"

#define INTF_NR 9
#define INTF_SCRIPT 0
#define INTF_PROMPT 1

char *prompts[][2] = {
    {"next_move" , "Your Turn! 轮到你走子了！"},
    {"set_start" , "Please set start position! 请摆出初始局面！"},
    {"fix_board" , "Please fix position! 请纠正棋盘！"},
    {"ill_move"  , "Illegal move! 非法着法"},
    {"opp_resign", "You win! Opponent resigned. 对手认输，你赢了！"},
    {"win"       , "You win! 你赢了！"},
    {"lose"      , "You lose! 你输了！"},
    {"draw"      , "Draw by rul! 和局！"},
    {"ill_start" , "Illegal start position! 非法初始局面！"}
};

int interface_init(void)
{
    struct passwd *pw = getpwuid(getuid());
    struct stat sts;
    const char *homedir = pw->pw_dir;

    mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    char dir_buf[1024];

    snprintf(dir_buf, 1024, "%s/.robotxq", homedir);
    if (stat(dir_buf, &sts) == -1 && errno == ENOENT) {
        if (mkdir(dir_buf, mode755)) {
            perror("mkdir");
            return -1;
        }
    }

    int cmd;
    for (cmd = 0; cmd < INTF_NR; ++cmd) {
        snprintf(dir_buf, 1024, "%s/.robotxq/%s.script", homedir, prompts[cmd][INTF_SCRIPT]);
        if (stat(dir_buf, &sts) == -1 && errno == ENOENT) {
            if (creat(dir_buf, mode755) < 0) {
                perror("create");
                return -1;
            }
            close(dir_buf);
        }
    }
    return 0;
}

int interface_prompt(enum intf cmd)
{
    puts(prompts[cmd][INTF_PROMPT]);

    struct passwd *pw = getpwuid(getuid());
    struct stat sts;
    const char *homedir = pw->pw_dir;

    char prog_buf[1024];
    char dir_buf[1024];

    snprintf(prog_buf, 1024, "%s/.robotxq/%s.script", homedir, prompts[cmd][INTF_SCRIPT]);
    snprintf(dir_buf, 1024, "%s/.robotxq", homedir);

    pid_t pid;

    if ((pid = fork()) < 0) {
        perror("fork");
        return -2;    /* fork error */
    }

    if (pid == 0) {    /* child */
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        chdir(dir_buf);
        if (execlp("sh", "sh", "-c", prog_buf, NULL) < 0) {
            perror("exec");
        }
    }

    return 0;
}
