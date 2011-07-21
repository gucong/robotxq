#ifndef _XIANGQI_H_
#define _XIANGQI_H_

#define VIRTPTS 90    /* number of points in virtual board */
#define PHYSPTS 96    /* number of points in physical board */
#define PIECES 32     /* number of pieces */

#define BD_SIZE 90
#define BD_FILES 9
#define BD_RANKS 10

#define BD_EMPTY     '_'
#define BD_W_PAWN    'P'    /* or Soldier */
#define BD_W_KNIGHT  'N'    /* or Horse(H) */
#define BD_W_BISHIP  'B'    /* or Elephant(E) */
#define BD_W_ROOK    'R'    /* or Chariot */
#define BD_W_ADVISOR 'A'    /* or Guard */
#define BD_W_KING    'K'    /* or General(G) */
#define BD_W_CANNON  'C'
#define BD_B_PAWN    'p'
#define BD_B_KNIGHT  'n'
#define BD_B_BISHIP  'b'
#define BD_B_ROOK    'r'
#define BD_B_ADVISOR 'a'
#define BD_B_KING    'k'
#define BD_B_CANNON  'c'
#define ALL_PIECES "_PNBRAKCpnbrakc"

#define BD_COL(i) ((i) % BD_FILES + 'a')
#define BD_ROW(i) ((i) / BD_FILES + '0')
#define BD_POS(col, row) (BD_FILES * ((row) - '0') + ((col) - 'a'))
#define BD_COLOR_PRED(c, COLOR)                 \
    ((c) == BD_ ## COLOR ## _PAWN   ||          \
     (c) == BD_ ## COLOR ## _KNIGHT ||          \
     (c) == BD_ ## COLOR ## _BISHIP ||          \
     (c) == BD_ ## COLOR ## _ROOK   ||          \
     (c) == BD_ ## COLOR ## _ADVISOR||          \
     (c) == BD_ ## COLOR ## _KING   ||          \
     (c) == BD_ ## COLOR ## _CANNON )
#define BD_COLOR(c) BD_COLOR_PRED((c),W) ? 'W' : 'B'
#define BD_PIECE_PRED(p, PIECE)                 \
    ((p) == BD_W_ ## PIECE || (p) == BD_B_ ## PIECE)
#define MOVE_FROM(move) BD_POS(move[0], move[1])
#define MOVE_TO(move) BD_POS(move[2], move[3])
#define MOVE_PROMOTE(move) ((move)[4])
#define MOVE_FROM_COL(move) ((move)[0])
#define MOVE_FROM_ROW(move) ((move)[1])
#define MOVE_TO_COL(move) ((move)[2])
#define MOVE_TO_ROW(move) ((move)[3])
#define TO_W toupper
#define TO_B tolower

char *fen_to_board(const char *fen_setup);
char *board_to_fen1(const char *board, char *fen_setup);

int extract_move(const char *prev_board, const char *cur_board, char *move);

char *apply_move(const char *prev_board, const char *move);

int diff_board(char *board1, char *board2);

#endif
