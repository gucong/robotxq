#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>

#include "read_board.h"
#include "xiangqi.h"

void print_help(char *argv0) {
    printf("Usage: %s [OPTION]... DEVICE\n", argv0);
    puts  ("catboard -- print board to stdout");
    puts  ("  -h            display this help and exit");
    puts  ("  -r READER     use READER as the program to read the board in");
    puts  ("  -b BRD_FILE   use BRD_FILE as the board configuration file");
    puts  ("  -n PASS       read PASS pass referencing BRD_FILE. (default 4)");
}

int main(int argc, char **argv)
{
    /* default argument */
    struct passwd *pw = getpwuid(getuid());
    struct stat sts;
    const char *homedir = pw->pw_dir;
    char brd_buf[1024];

    /* if not work with autotools */
#ifndef CONFDIR
#define CONFDIR "/etc"
#endif

    snprintf(brd_buf, 1024, "%s/.robotxq/xq.brd", homedir);
    if (stat(brd_buf, &sts) == -1 && errno == ENOENT)
        strncpy(brd_buf, CONFDIR "/xq.brd", 1024);

    char *reader = "io_board";
    char *brd_file = brd_buf;
    int pass = 4;

    /* argument parsing */
    int opt;
    while ((opt = getopt(argc, argv, "hr:b:n:")) != -1) {
        switch (opt) {
        case 'r':
            reader = optarg;
            break;
        case 'b':
            brd_file = optarg;
            break;
        case 'n':
            pass = atoi(optarg);
            break;
        case 'h':
            print_help(argv[0]);
            return 0;
        default:
            print_help(argv[0]);
            return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "expect device node after options\n");
        print_help(argv[0]);
        return 1;
    }

    char *dev = argv[optind];

    /* read board */
    char reader_buf[1024];
    snprintf(reader_buf, 1024, "%s %s", reader, dev);
    if (init_read_board(reader_buf, brd_file) != 0)
        return 1;

    char board[90];
    read_board(board, pass);

    /* memcpy(board, */
    /*        fen_to_board("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1") */
    /*        , BD_SIZE); */

    /* print board */
    int i, j;
    printf("\n");
    for (i = BD_RANKS - 1; i >= 0; --i) {
        for (j = 0; j < BD_FILES; ++j) {
            printf("%c", board[i*BD_FILES+j]);
        }
        printf("\n");
    }

    printf("\n");
    printf("%s\n", board_to_fen1(board));

    return 0;
}    
