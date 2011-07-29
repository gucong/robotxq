#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <sys/wait.h>

#define MAXLINE 4096
#define TIMEOUT 2    /* poll timeout in seconds */

/* choose variant */
#define GAME_XIANGQI

#ifdef GAME_CHESS
#include "chess.h"
#endif

#ifdef GAME_XIANGQI
#include "xiangqi.h"
#endif

#include "read_board.h"
#include "cpopen.h"

struct termios old_tio;
pid_t handctl_pid;

/* extensible human interface */
/* ----------------------------------- */
void prompt_human(void)
{
    puts("Your Turn! 轮到你走子了！");
}

void prompt_setboard(void)
{
    puts("Please setup initial board! 请摆开局！");
}

void prompt_fixboard(void)
{
    puts("Please fix board position! 请纠正棋盘！");
}

/* ----------------------------------- */

void wait_human()
{
    int c;
    printf("waiting human ...\n");
    read(STDIN_FILENO, &c, 1);
}    

void wait_phyboard(FILE *from_handctl)
{
    char buf[6];
    printf("waiting machine...\n");
    fgets(buf, 5, from_handctl);
}

void print_board(char *board) {
    int i, j;
    printf("\n");
    for (i = BD_RANKS - 1; i >= 0; --i) {
        for (j = 0; j < BD_FILES; ++j) {
            printf("%c", board[i*BD_FILES+j]);
        }
        printf("\n");
    }
    printf("\n");
}

char *read_phyboard(int pass)
{
    static char board[BD_SIZE];
    if (!pass)
        return board;
    if (read_board(board, pass) == -1)
        fprintf(stderr, "warning: unable to read board\n");
    printf("board is read\n");
    return board;
}

int one_chess_game (char *fen_setup, char *engine, FILE **handctl_fp)
{
    FILE *fp[2];

    pid_t engine_pid;
    if ((engine_pid = cpopen(fp, engine)) < 0) {
        perror("coprocess open");
        return 0;
    }

    FILE *from_engine = fp[0];
    FILE *to_engine = fp[1];
    int fd_from_engine = fileno(from_engine);
    FILE *from_handctl = handctl_fp[0];
    FILE *to_handctl = handctl_fp[1];


    int usermove = 0, getboard = 0;
    struct pollfd engine_poll = {
        .fd = fd_from_engine,
        .events = POLLIN
    };


    /* init engine */
    fputs("xboard\nprotover 2\n", to_engine);
    char line[MAXLINE];
    char *token;
    int timeout = TIMEOUT * 1000;
    int done = 0;
    int ret;
    while (!done) {
        /* read features */
        ret = poll(&engine_poll, 1, timeout);
        if (ret == -1) {
            /* error occurred */
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else
                perror("poll");
        }
        else if (!ret)
            /* no feature response, go ahead */
            done = 1;
        else if (engine_poll.revents & POLLIN) {
            fgets(line, MAXLINE, from_engine);
            token = strtok(line, " \n");
            int head = 1;
            while (token) {
                /* respond to features */
                if (head == 1 && strcmp(token, "feature") != 0) {
                    break;
                } else if (strcmp(token, "done=0") == 0) {
                    timeout = 3600000;
                } else if (strcmp(token, "done=1") == 0) {
                    done = 1;
                    break;
                } else if (strcmp(token, "usermove=1") == 0) {
                    usermove = 1;
                } else if (strcmp(token, "getboard=1") == 0) {
                    getboard = 1;
                }
                token = strtok(NULL, " \n");
                head = 0;
            }
        }
    }

    fputs("accepted setboard\n", to_engine);    /* require setboard */
#ifdef GAME_XIANGQI
    fputs("variant xiangqi", to_engine);
#endif
    fputs("st 7\n", to_engine);    /* time control */
    fputs("hard\n", to_engine);    /* pondering if capable */

    /* pick up side */
    char human_color = 'w';

    /* read physical board */
    int first_move = 1;
    char *cur_phyboard;
    char prev_phyboard[BD_SIZE];
    prompt_setboard();
    wait_human();
    snprintf(fen_setup, 127, "%s %c - - 0 1",
             board_to_fen1((cur_phyboard = read_phyboard(10))),
             human_color);
    print_board(read_phyboard(0));
    strncpy(prev_phyboard, cur_phyboard, BD_SIZE);

    /* engine setup board */
    fprintf(to_engine, "setboard %s", fen_setup);
    if (poll(&engine_poll, 1, 0) == -1)
        perror("poll");
    else if (engine_poll.revents & POLLIN) {
        fgets(line, MAXLINE, from_engine);
        if (strncmp(line, "Illegal", 7) == 0) {
            kill(engine_pid, SIGTERM);
            wait(NULL);
            return 6;    /* illegal initial position */
        }
    }

    fputs("force\n", to_engine);

    /* main loop */
    while (1) {
        char *move;
        char emove[6];
        char *board;
        int ret, cnt1, cnt2;
        int thres1 = 5, thres2 = 3;

        /* ask human player to go */
        prompt_human();
        wait_human();
        cnt1 = cnt2 = 0;
        while ((ret = extract_move(prev_phyboard, (cur_phyboard = read_phyboard(4)), emove))
               != 0) {
            switch (ret) {
            case 1:
                puts("possible read board error, dump board:");
                print_board(read_phyboard(0));
                ++cnt1;
                if (cnt1 < thres1)    /* automatically read thres1 times */
                    break;
            case 2:
                puts("illegal position, dump board:");
                print_board(read_phyboard(0));
                ++cnt2;
                if (cnt2 >= thres2) {
                    kill(engine_pid, SIGTERM);
                    wait(NULL);
                    return 1;    /* Illegal physical move, return */
                }
                prompt_fixboard();
                wait_human();
                break;
            }
        }

        /* send physical move to engine and get result */
        fprintf(to_engine, "%s%s\n", usermove?"usermove ":"", emove);

        if (first_move) {
            fputs("go\n", to_engine);
            first_move = 0;
        }

        fgets(line, MAXLINE, from_engine);

        printf("engine<<  move %s\n", emove);
        printf("engine>>  %s\n", line);

        token = strtok(line, " \n");
        if (strcmp(token, "move") == 0) {
            /* apply move to physical board */
            move = strtok(NULL, " \n");
            board = apply_move_phyboard(cur_phyboard, move, to_handctl);
            puts("setting the board");
            print_board(board);
            wait_phyboard(from_handctl);
            while (diff_board((cur_phyboard = read_phyboard(4)), board)) {
                puts("possible read board error, dump board:");
                print_board(read_phyboard(0));
                puts("expected board:");
                print_board(board);
            }
            strncpy(prev_phyboard, cur_phyboard, BD_SIZE);
        }
        else if (strcmp(token, "Illegal") == 0) {
            kill(engine_pid, SIGTERM);
            wait(NULL);
            return 1;    /* illegal physical move */
        }
        else if (strcmp(token, "resign") == 0) {
            kill(engine_pid, SIGTERM);
            wait(NULL);
            return 2;    /* machine resign, human win */
        }
        else if (strcmp(token, "1-0") == 0) {
            kill(engine_pid, SIGTERM);
            wait(NULL);
            if (human_color == 'b')
                return 4;    /* human lose */
            else
                return 3;    /* human win */
        }
        else if (strcmp(token, "0-1") == 0) {
            kill(engine_pid, SIGTERM);
            wait(NULL);
            if (human_color == 'b')
                return 3;    /* human win */
            else
                return 4;    /* human lose */
        }
        else if (strcmp(token, "1/2-1/2") == 0) {
            kill(engine_pid, SIGTERM);
            wait(NULL);
            return 5;    /* draw by rule */
        }
    }
}

/* sig handler to restore stdin */
static void sig_handler(int signo)
{
    kill(handctl_pid, SIGTERM);
    wait(NULL);
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old_tio) == -1) {
        perror("stdin tcgetattr");
        exit(EXIT_FAILURE);
    }
    tcflush(STDIN_FILENO, TCOFLUSH);
    tcflush(STDIN_FILENO, TCIFLUSH);
    exit(EXIT_SUCCESS);
}

int tcset_noncanonical(int fd)
{
    struct termios tio;

    if (tcgetattr(fd, &old_tio) == -1) {
        perror("tcgetattr");
        return 1;
    }
    memcpy(&tio, &old_tio, sizeof(struct termios));

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("register SIGTERM");
        return 1;
    }

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("register SIGINT");
        return 1;
    }

    tio.c_lflag |= ISIG;
    tio.c_lflag &= ~(ICANON | ECHO);
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VINTR] = 3;
    if (tcsetattr(fd, TCSANOW ,&tio) == -1) {
        perror("tcsetattr");
        return 1;
    }
    tcflush(fd, TCOFLUSH);
    tcflush(fd, TCIFLUSH);
    return 0;
}

void print_help(char *argv0)
{
    printf("Usage: %s [OPTION]... BOARD_DEV HAND_DEV\n", argv0);
    puts  ("robotxq -- Robot plays Xiangqi");
    puts  ("OPTION:");
    puts  ("  -h            display this help and exit");
    puts  ("  -e ENGINE     use ENGINE as the engine program");
    puts  ("  -r READER     use READER as the program to read the board in");
    puts  ("  -w HANDCTL    use HANDCTL as the hand controler");
    puts  ("  -b BRD_FILE   use BRD_FILE as the board configuration file");
}

int main (int argc, char *argv[])
{
    //srandom(time(NULL));

    /* default arguments */
    struct passwd *pw = getpwuid(getuid());
    struct stat sts;
    const char *homedir = pw->pw_dir;
    char brd_buf[1024];

#ifndef CONFDIR     /* if not working with autotools */
#define CONFDIR "/etc"
#endif

    snprintf(brd_buf, 1024, "%s/.robotxq/xq.brd", homedir);
    if (stat(brd_buf, &sts) == -1 && errno == ENOENT)
        strncpy(brd_buf, CONFDIR "/xq.brd", 1024);
    
    char *engine = "eleeye_xb";
    char *reader = "io_board";
    char *handctl = "io_hand";
    char *brd_file = brd_buf;

    /* argument parsing */
    int opt;
    while ((opt = getopt(argc, argv, "he:r:w:b:")) != -1) {
        switch (opt) {
        case 'e':
            engine = optarg;
            break;
        case 'r':
            reader = optarg;
            break;
        case 'w':
            handctl = optarg;
            break;
        case 'b':
            brd_file = optarg;
            break;
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind + 1 >= argc) {
        fprintf(stderr, "expect device node after options\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *board_dev = argv[optind];
    char *hand_dev = argv[optind+1];

    /* init board reader */
    char reader_buf[1024];
    snprintf(reader_buf, 1024, "%s %s", reader, board_dev);
    if (init_read_board(reader_buf, brd_file) != 0)
        exit(EXIT_FAILURE);

    /* init hand controler */
    char handctl_buf[1024];
    snprintf(handctl_buf, 1024, "%s %s", handctl, hand_dev);
    FILE *handctl_fp[2];
    if ((handctl_pid = cpopen(handctl_fp, handctl_buf)) < 0)
        exit(EXIT_FAILURE);

    /* set stdin to non-canonical mode */
    if (tcset_noncanonical(STDIN_FILENO) != 0)
        exit(EXIT_FAILURE);

    char fen_setup[128];
    /* main loop */
    while(1) {
        switch (one_chess_game(fen_setup, engine, handctl_fp)) {
        case 1:
            printf("You lose! Illegal move.\n");
            puts("dump board:");
            print_board(read_phyboard(0));
            break;
        case 2:
            printf("You win! Machine resigned.\n");
            break;
        case 3:
            printf("You win!\n");
            break;
        case 4:
            printf("You lose!\n");
            break;
        case 5:
            printf("Draw by rule...\n");
            break;
        case 6:
            printf("Illegal initial position: %s\n", fen_setup);
            puts("dump board:");
            print_board(read_phyboard(0));
            break;
        }
    }
}
