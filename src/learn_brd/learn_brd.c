#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

#define VIRTPTS 90    /* number of points in virtual board */
#define PHYSPTS 96    /* number of points in physical board */
#define PORTLEN 96    /* number of integers read from port */
#define PIECES 32    /* number of pieces */
const int start_virt_pos[PIECES] = {
    54, 56, 58, 60, 62,    /* ppppp */
    64, 70,    /* cc */
    81, 82, 83, 84, 85, 86, 87, 88, 89,    /* rnbakabnr */
    27, 29, 31, 33, 35,    /* PPPPP */
    19, 25,    /* CC */
    0,  1,  2,  3,  4,  5,  6,  7,  8      /* RNBAKABNR */
};

#define MAXLINE 4096

char prog[1024];

unsigned int *read_port();

void print_help(argv0)
{
    printf("Usage: %s [OPTION]... DEVICE\n", argv0);
    puts  ("learn_brd -- learn board and/or pieces interactively");
    puts  ("OPTION:");
    puts  ("  -h            display this help and exit");
    puts  ("  -P            learn pieces");
    puts  ("  -B            learn board");
    puts  ("  -i BRD_IN     use BRD_IN for input, with -P");
    puts  ("                if -B is specified, it is ignored");
    puts  ("  -o BRD_OUT    use BRD_OUT for output");
    puts  ("  -r READER     use READER as the program to read the board in");
    puts  ("  -n PASS       read PASS pass for piece learning,  with -P (default 5)");
}

int main(int argc, char **argv)
{
    /* default argument */
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

    char *reader = "io_board";
    char *brd_in = brd_buf;
    char *brd_out = NULL;
    int P = 0, B = 0;
    int pass_nr = 5;

    /* argument parsing */
    int opt;
    while ((opt = getopt(argc, argv, "hPBi:o:r:n")) != -1) {
        switch (opt) {
        case 'P':
            P = 1;
            break;
        case 'B':
            B = 1;
            break;
        case 'i':
            brd_in = optarg;
            break;
        case 'o':
            brd_out = optarg;
            break;
        case 'r':
            reader = optarg;
            break;
        case 'n':
            pass_nr = atoi(optarg);
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
    snprintf(prog, 1024, "%s %s", reader, dev);

    /* data to be gathered */
    int virt_to_phys[VIRTPTS];
    char order[] = "pppppccrnbakabnrPPPPPCCRNBAKABNR";
    int ordered_pieces[PIECES];

    /* learn board */
    int idx, ret;
    if (B) {
        printf("learn board\n");
        for (idx = 0; idx < VIRTPTS; ++idx) {
            printf("read position: %d\n", idx);

            while ((ret = find_nonzero(read_port(), PORTLEN)) == -1)
                printf("fail, read again position: %d\n", idx);

            virt_to_phys[idx] = ret;
            printf("virt: %5d;  phys: %5d\n==================\n", idx, ret);
        }
        printf("board learning finished, saving...\n");
    }
    else {
        /* don't learn board, but read from brd_in */
        printf("read from %s\n", brd_in);
        FILE *in_fp = fopen(brd_in, "r");
        if (in_fp == NULL) {
            perror("open brd_in");
            return 1;
        }

        char buf[MAXLINE];
        char *ptr;
        int i;
        fgets(buf, MAXLINE, in_fp);
        for (ptr = strtok(buf, " \n"), i = 0; ptr && i < VIRTPTS; ptr = strtok(NULL, " \n"), ++i) {
            virt_to_phys[i] = atoi(ptr);
        }

        if (i != VIRTPTS) {
            printf("warning: %s:1: less than %d entries\n", brd_in, VIRTPTS);
        }
    }

    /* save board config */
    FILE *fp;
    if (brd_out)
        fp = fopen(brd_out, "w");
    else
        fp = stdout;
    if (fp == NULL) {
        perror("open brd_out");
        return 1;
    }
    for (idx = 0; idx < VIRTPTS; ++idx) {
        fprintf(fp, "%d ", virt_to_phys[idx]);
    }
    fprintf(fp, "\n");
    printf("\n===================================\nok\n\n");

    /* learn pieces */
    int step = PORTLEN;
    int *data = malloc(sizeof(int) * step * pass_nr);
    if (P) {
        printf("learn pieces\n");

        /* get data */
        int pass;
        for (pass = 0; pass < pass_nr; ++pass) {
            printf("pass %d, please ensure start position, \n", pass);
            memcpy(data + step * pass, read_port(), sizeof(int) * step);
            printf("pass %d saved\n==================\n");
        }

        /* do statistics */
        printf("doing statistics\n");
        int piece;
        int x, y, y2;
        int max_val, max_times;
        for (piece = 0; piece < PIECES; ++piece) {
            max_times = 0;
            max_val = 0;
            x = virt_to_phys[start_virt_pos[piece]];
            for (y = 0; y < pass_nr; ++y) {
                int val = (data + y * step)[x];
                int times = 1;
                if (val == 0) continue;
                for (y2 = y; y2 < pass_nr; ++y2) {
                    if (val == (data + y2 * step)[x]) {
                        ++times;
                        (data + y2 * step)[x] = 0;
                    }
                }
                if (times > max_times) {
                    max_times = times;
                    max_val = val;
                }
            }
            if (max_val == 0) {
                ordered_pieces[piece] = 0;
                printf("position %d, piece %c : no data!!!\n==================\n",
                       start_virt_pos[piece], order[piece]);
            }
            else {
                ordered_pieces[piece] = max_val;
                printf("position %d, piece %c : %d\n==================\n",
                       start_virt_pos[piece], order[piece], max_val);
            }
        }

        /* save piece config */
        printf("piece learning finished, saving...\n");
        fprintf(fp, order);
        fprintf(fp, "\n");
        for (piece = 0; piece < PIECES; ++piece) {
            fprintf(fp, "%d ", ordered_pieces[piece]);
        }
        fprintf(fp, "\n");
        printf("\n===================================\nok\n\n");
    }
    fclose(fp);
    //    free(data);

    return 0;

}

int find_nonzero(int *dat, int nr)
{
    int idx;
    for (idx = 0; idx < nr; ++idx) {
        if (dat[idx])
            return idx;
    }
    return -1;
}

unsigned int *read_port()
{
    static unsigned int buffer[PORTLEN];
    FILE * fp;

    /* wait a keyboard stroke */
    printf("wait a key...\n");
    getchar();

    if ((fp = popen(prog, "r")) <= 0) {
        perror("popen");
        return NULL;
    }

    fread(buffer, 4, PORTLEN, fp);

    pclose(fp);

    return buffer;
}
