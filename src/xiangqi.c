#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xiangqi.h"

static void set_move(char *move, char cor1, char cor2)
{
    *move++ = BD_COL(cor1);
    *move++ = BD_ROW(cor1);
    *move++ = BD_COL(cor2);
    *move++ = BD_ROW(cor2);
    *move = 0;
}

char *fen_to_board(const char *fen_setup)
{
    static char board[BD_SIZE];
    /* xiangqi board:
          +---------------------+
       9  |  81  ...         89 |
       8  |  72  ...            |
       .  |  ...                |
       .  |  ...                |
       1  |  9  10  ...         |
       0  |  0  1  2   ...   8  |
          +---------------------+
             a  b  c   ...   i
    */
    if (! fen_setup)
        return board;
    int p = 0;
    int i = BD_SIZE - BD_FILES;
    int j;
    for (;; ++p) {
        switch (fen_setup[p]) {
        case '1' ... ('0' + BD_FILES):
            for (j = '0'; j < fen_setup[p]; ++j)
                board[i++] = BD_EMPTY;
            break;
        case '/':
            if (i % BD_FILES != 0) {
                printf("invalid fen notation: %s\n", fen_setup);
                return NULL;
            }
            i -= 2 * BD_FILES;
            break;
        case BD_W_PAWN   :
        case BD_W_KNIGHT :
        case BD_W_BISHIP :
        case BD_W_ROOK   :
        case BD_W_ADVISOR:
        case BD_W_KING   :
        case BD_W_CANNON :
        case BD_B_PAWN   :
        case BD_B_KNIGHT :
        case BD_B_BISHIP :
        case BD_B_ROOK   :
        case BD_B_ADVISOR:
        case BD_B_KING   :
        case BD_B_CANNON :
            board[i++] = fen_setup[p];
            break;
        default:
            if (i != BD_FILES) {
                printf("invalid fen notation: %s\n", fen_setup);
                return NULL;
            }
            return board;
        }
    }
}

int extract_move(const char *prev_board, const char *cur_board, char *move)
{
    int gone[1], appear[1], change[1];
    int gone_nr = 0, appear_nr = 0, change_nr = 0;
    int i;
    for (i = 0; i < BD_SIZE; ++i) {
        char prev = prev_board[i];
        char cur = cur_board[i];
        if (prev != cur) {
            if (prev == BD_EMPTY && cur != BD_EMPTY) {
                if (appear_nr == 0)
                    appear[appear_nr] = i;
                appear_nr++;
            }
            else if (prev != BD_EMPTY && cur == BD_EMPTY) {
                if (gone_nr == 0)
                    gone[gone_nr] = i;
                gone_nr++;
            }
            else {
                if (change_nr == 0)
                    change[change_nr] = i;
                change_nr++;
            }
        }
    }

    if (gone_nr == 1 && appear_nr == 1 && change_nr == 0) {
        if (prev_board[gone[0]] == cur_board[appear[0]]) {
            /* move */
            set_move(move, gone[0], appear[0]);
            return 0;
        }
        else
            return 2;
    }
    else if (gone_nr == 1 && appear_nr == 0 && change_nr == 1) {
        if (BD_COLOR(prev_board[change[0]]) != BD_COLOR(cur_board[change[0]])) {
            /* capture */
            set_move(move, gone[0], change[0]);
            return 0;
        }
        else
            return 2;
    }
    else if (gone_nr > 1 && appear_nr == 1 && change_nr == 0 ||
             gone_nr > 1 && appear_nr == 0 && change_nr == 1 ||
             gone_nr > 0 && appear_nr == 0 && change_nr == 0)
        /* possible read board error */
        return 1;
    else
        return 2;
}

char *apply_move(const char *prev_board, const char *move)
{
    static char board[BD_SIZE];
    if (board == NULL || move == NULL)
        return board;
    strncpy(board, prev_board, BD_SIZE);
    board[MOVE_FROM(move)] = BD_EMPTY;
    /* move / capture */
    board[MOVE_TO(move)] = prev_board[MOVE_FROM(move)];
    return board;
}

inline int diff_board(char *board1, char *board2)
{
    return strncmp(board1, board2, BD_SIZE);
}
