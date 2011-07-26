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

#define MAXLINE 4096
#define TIMEOUT 2    /* poll timeout in seconds */

/* choose variant */
//#define GAME_CHESS
#define GAME_XIANGQI

#ifdef GAME_CHESS
#include "chess.h"
#endif

#ifdef GAME_XIANGQI
#include "xiangqi.h"
#endif

#include "read_board.h"

struct termios old_tio;

void prompt_human(void)
{
    printf("Your Turn.\n");
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
}

int set_phyboard(char *board)
{
    /////////////
    /* send request */
    /* read request */
    /* place pieces */
    /////////////
    /* the following implementation is for testing */
    printf("set physical board\n");
    print_board(board);
    return 0;
}

/* char *wait_phyboard(1) */
/* { */
/*     static char board[BD_SIZE]; */
/*     ///////////// */
/*     /\* the following implementation is for testing without physical board*\/ */
/*     printf("wait physical board\n"); */
/*     int i, j, c; */
/*     for (i = BD_RANKS - 1; i >= 0; --i) { */
/*         for (j = 0; j < BD_FILES; ++j) { */
/*             c = getchar(); */
/*             while (strchr(ALL_PIECES, c) == NULL) */
/*                 c = getchar(); */
/*             board[i*BD_FILES+j] = c; */
/*         } */
/*     } */
/*     getchar(); */
/*     return board; */
/* } */

void prompt_wait()
{
    int c;
    printf("wait physical board ...\n");
    read(STDIN_FILENO, &c, 1);
}    

char *read_phyboard(int read)
{
    static char board[BD_SIZE];
    if (!read)
        return board;
    if (read_board(board, 4) == -1)
        printf("warning: unable to read physical board");
    printf("board read\n");
    return board;
}

int one_chess_game (char *fen_setup, char *engine)
{
    int fd1[2], fd2[2];
    pid_t pid;

    /* open pipes */
    if (pipe(fd1) < 0 || pipe(fd2) < 0) {
        perror("pipe");
        return 1;
    }

    /* fork */
    if ((pid = fork()) < 0)
        perror("fork");
    else if (pid > 0) {
        /* parent */
        /* deal with fildes and filptr */
        close(fd1[0]);
        close(fd2[1]);
        int fd_from_engine = fd2[0];
        int fd_to_engine = fd1[1];
        FILE *from_engine = fdopen(fd_from_engine, "r");
        FILE *to_engine = fdopen(fd_to_engine, "w");
        int usermove = 0, getboard = 0;
        if (setvbuf(to_engine, NULL, _IONBF, 0) != 0)
            perror("setvbuf");
        if (setvbuf(from_engine, NULL, _IONBF, 0) != 0)
            perror("setvbuf");
        struct pollfd engine_poll = {
            .fd = fd_from_engine,
            .events = POLLIN
        };

        /* setup physical board */
        char human_color = *(strchr(fen_setup, ' ') + 1);
        set_phyboard(fen_to_board(fen_setup));

        /* setup engine */
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
        fputs("st 10\n", to_engine);    /* time control */
        fputs("hard\n", to_engine);    /* pondering if possible */

        fprintf(to_engine, "setboard %s", fen_setup);    /* engine board setup */
        if (poll(&engine_poll, 1, 0) == -1)
            perror("poll");
        else if (engine_poll.revents & POLLIN) {
            fgets(line, MAXLINE, from_engine);
            if (strncmp(line, "Illegal", 7) == 0) {
                fputs("quit\n", to_engine);
                return 6;    /* illegal initial position */
            }
        }

        fputs("force\n", to_engine);

        /* verify physical setup */
        int first_move = 1;
        char *cur_phyboard;
        char prev_phyboard[BD_SIZE];
        prompt_wait();
        while (diff_board((cur_phyboard = read_phyboard(1)), fen_to_board(NULL))) {
            puts("possible read board error, dump board:");
            print_board(read_phyboard(0));
            set_phyboard(fen_to_board(NULL));
        }
        strncpy(prev_phyboard, cur_phyboard, BD_SIZE);

        while (1) {
            char *move;
            char emove[6];
            char *board;
            int ret;
            /* ask human player to go */
            prompt_human();
        
            /* generate physical move */
            prompt_wait();
            while ((ret = extract_move(prev_phyboard, (cur_phyboard = read_phyboard(1)), emove)) == 1) {
                puts("possible read board error, dump board:");
                print_board(read_phyboard(0));
            }
            if (ret == 2) {
                fputs("quit\n", to_engine);
                return 1;    /* illegal physical move */
            }

            /* send physical move to engine and get result */
            fprintf(to_engine, "%s%s\n", usermove?"usermove ":"", emove);
            if (first_move) {
                fputs("go\n", to_engine);
                first_move = 0;
            }
            fgets(line, MAXLINE, from_engine);
            ///////////////////debug
            printf("engine<<  move %s\n", emove);
            printf("engine>>  %s\n", line);
            ///////////////////
            token = strtok(line, " \n");
            if (strcmp(token, "move") == 0) {
                if (getboard) {
                    fputs("getboard\n", to_engine);
                    fgets(line, MAXLINE, from_engine);
                    board = fen_to_board(line);
                } else {
                    move = strtok(NULL, " \n");
                    board = apply_move(cur_phyboard, move);
                }
                set_phyboard(board);
                prompt_wait();
                while (diff_board((cur_phyboard = read_phyboard(1)), board)) {
                    puts("possible read board error, dump board:");
                    print_board(read_phyboard(0));
                    set_phyboard(board);
                }
                strncpy(prev_phyboard, cur_phyboard, BD_SIZE);
            }
            else if (strcmp(token, "Illegal") == 0) {
                fputs("quit\n", to_engine);
                return 1;    /* illegal physical move */
            }
            else if (strcmp(token, "resign") == 0) {
                fputs("quit\n", to_engine);
                return 2;    /* machine resign, human win */
            }
            else if (strcmp(token, "1-0") == 0) {
                fputs("quit\n", to_engine);
                if (human_color == 'b')
                    return 4;    /* human lose */
                else
                    return 3;    /* human win */
            }
            else if (strcmp(token, "0-1") == 0) {
                fputs("quit\n", to_engine);
                if (human_color == 'b')
                    return 3;    /* human win */
                else
                    return 4;    /* human lose */
            }
            else if (strcmp(token, "1/2-1/2") == 0) {
                fputs("quit\n", to_engine);
                return 5;    /* draw by rule */
            }
        }
    }

    else {
        /* child */
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
        if (execlp("sh", "sh", "-c", engine, NULL) < 0) {
            perror("exec");
        }
    }
    return 0;    /* not reached */
}

/* sig handler to restore stdin */
static void sig_handler(int signo)
{
    if (tcgetattr(STDIN_FILENO, &old_tio) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    tcflush(STDIN_FILENO, TCOFLUSH);
    tcflush(STDIN_FILENO, TCIFLUSH);
    exit(EXIT_SUCCESS);
}

void print_help(char *argv0)
{
    printf("Usage: %s [OPTION]... DEVICE\n", argv0);
    puts  ("robotxq -- Robot plays Xiangqi");
    puts  ("OPTION:");
    puts  ("  -h            display this help and exit");
    puts  ("  -e ENGINE     use ENGINE as the engine program");
    puts  ("  -f FEN_FILE   use positions in FEN_FILE as start positions");
    puts  ("  -r READER     use READER as the program to read the board in");
    puts  ("  -b BRD_FILE   use BRD_FILE as the board configuration file");
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
    if (tcsetattr(fd, TCSANOW ,&tio) == -1) {
        perror("tcsetattr");
        return 1;
    }
    tcflush(fd, TCOFLUSH);
    tcflush(fd, TCIFLUSH);
    return 0;
}

int main (int argc, char *argv[])
{
    //srandom(time(NULL));

    /* default arguments */
    struct passwd *pw = getpwuid(getuid());
    struct stat sts;
    const char *homedir = pw->pw_dir;
    char fen_buf[1024];
    char brd_buf[1024];

#ifndef CONFDIR     /* if not working with autotools */
#define CONFDIR "/etc"
#endif

    snprintf(fen_buf, 1024, "%s/.robotxq/xq.fen", homedir);
    if (stat(fen_buf, &sts) == -1 && errno == ENOENT)
        strncpy(fen_buf, CONFDIR "/xq.fen", 1024);

    snprintf(brd_buf, 1024, "%s/.robotxq/xq.brd", homedir);
    if (stat(brd_buf, &sts) == -1 && errno == ENOENT)
        strncpy(brd_buf, CONFDIR "/xq.brd", 1024);
    
    char *engine = "eleeye_xb";
    char *reader = "io_board";
    char *fen_file = fen_buf;
    char *brd_file = brd_buf;

    /* argument parsing */
    int opt;
    while ((opt = getopt(argc, argv, "he:f:r:b:")) != -1) {
        switch (opt) {
        case 'e':
            engine = optarg;
            break;
        case 'f':
            fen_file = optarg;
            break;
        case 'r':
            reader = optarg;
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

    if (optind >= argc) {
        fprintf(stderr, "expect device node after options\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *dev = argv[optind];

    /* printf("ENGINE:   %s\nREADER:   %s\nFEN_FILE: %s\nBRD_FILE: %s\n", */
    /*        engine, reader, fen_file, brd_file); */

    /* loading FEN_FILE */
    FILE *fen_fp = fopen(fen_file, "r");
    if (fen_fp == NULL) {
        perror("open fen_file");
        exit(EXIT_FAILURE);
    }
    char fen_setup[128];
    int board_nr, board_idx;
    for (board_nr = 0; fgets(fen_setup, 126, fen_fp); ++board_nr)
        if (strncmp(fen_setup, "end", 3) == 0)
            break;
    long *line_offset = (long *)malloc(board_nr * sizeof(long));
    fseek(fen_fp, 0, SEEK_SET);
    for (board_idx = 0; board_idx < board_nr; ++board_idx) {
        line_offset[board_idx] = ftell(fen_fp);
        fgets(fen_setup, 126, fen_fp);
    }

    /* init board reader */
    char reader_buf[1024];
    snprintf(reader_buf, 1024, "%s %s", reader, dev);
    if (init_read_board(reader_buf, brd_file) != 0)
        exit(EXIT_FAILURE);

    /* set stdin to non-canonical mode */
    if (tcset_noncanonical(STDIN_FILENO) != 0)
        exit(EXIT_FAILURE);

    /* main loop */
    while(1) {
        board_idx = random() % board_nr;
        fseek(fen_fp, line_offset[board_idx], SEEK_SET);
        fgets(fen_setup, 126, fen_fp);
        switch (one_chess_game(fen_setup, engine)) {
        case 1:
            printf("You lose! Illegal move.\n");
            puts("dump board:");
            print_board(read_phyboard(0));
            break;
        case 2:
            printf("You win! Machine resgined.\n");
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
            printf("Illegal initial position: %s", fen_setup);
            break;
        }
    }
}
