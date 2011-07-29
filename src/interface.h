#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#define INTF_NR 9
#define INTF_SCRIPT 0
#define INTF_PROMPT 1

enum intf {
    NEXT_MOVE,
    SET_START,
    FIX_BOARD,
    ILL_MOVE,
    OPP_RESIGN,
    WIN,
    LOSE,
    DRAW,
    ILL_START
};

int interface_init(void);

int interface_prompt(enum intf cmd);

#endif
