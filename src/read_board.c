#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_board.h"
#include "xiangqi.h"

#define MAXLINE 1024

char read_board_prog[MAXLINE];
int virt_to_phys[VIRTPTS];
char order[PIECES];
int ordered_pieces[PIECES];

static char lookup(int content);

int init_read_board(const char *prog, const char *conf_file)
{
    /* save prog */
    strncpy(read_board_prog, prog, MAXLINE);

    /* load board configuration into memory*/
    char buf[MAXLINE];
    char *ptr;
    int i;
    FILE *conf_fp = fopen(conf_file, "r");

    fgets(buf, MAXLINE, conf_fp);
    for (ptr = strtok(buf, " \n"), i = 0; ptr && i < VIRTPTS; ptr = strtok(NULL, " \n"), ++i) {
        virt_to_phys[i] = atoi(ptr);
    }
    if (i != VIRTPTS) {
        printf("warning: %s:1: less than %d entries\n", conf_file, VIRTPTS);
    }

    fgets(buf, MAXLINE, conf_fp);
    strncpy(order, buf, PIECES);
    if (strlen(buf) < PIECES) {
        printf("warning: %s:2: less than %d entries\n", conf_file, PIECES);
    }

    fgets(buf, MAXLINE, conf_fp);
    for (ptr = strtok(buf, " \n"), i = 0; ptr && i < PIECES; ptr = strtok(NULL, " \n"), ++i) {
        ordered_pieces[i] = atoi(ptr);
    }
    if (i != PIECES) {
        printf("warning: %s:3: less than %d entries\n", conf_file, PIECES);
    }

    fclose(conf_fp);

    return 0;
}

int read_board(char *board, int pass_nr)
{
    memset(board, '_', BD_SIZE);
    /* note: 'board' assumes to be in BD_SIZE */

    FILE *fp;
    int pass;
    for (pass = 0; pass < pass_nr; ++pass) {
        /* read raw data */
        int phys_data[PHYSPTS];
        if ((fp = popen(read_board_prog, "r")) <= 0) {
            perror("popen");
            return -1;
        }

        fread(phys_data, 4, PHYSPTS, fp);

        pclose(fp);

        /* translate */
        int vidx, content;
        char c;
        for (vidx = 0; vidx < VIRTPTS; ++vidx) {
            content = phys_data[virt_to_phys[vidx]];
            if (content && (c = lookup(content))) {
                board[vidx] = c;
            }
            else if (content) {
                printf("warning: lookup failed (vidx %d, content %d)\n", vidx, content);
            }
        }
    }

    return 0;
}

static char lookup(int content)
{
    int i;
    for (i = 0; i < PIECES; ++i) {
        if (ordered_pieces[i] == content)
            return order[i];
    }
    return 0;
}
