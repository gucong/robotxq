#ifndef _INTERFACE_H_
#define _INTERFACE_H_

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

extern pid_t interface_pid;

int interface_init(void);

int interface_prompt(enum intf cmd);

#endif
