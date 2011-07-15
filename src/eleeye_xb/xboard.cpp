/*
ucci.h/ucci.cpp - Source Code for ElephantEye, Part I

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.15, Last Modified: Jul. 2008
Copyright (C) 2004-2008 www.elephantbase.net

This part (ucci.h/ucci.cpp only) of codes is NOT published under LGPL, and
can be used without restriction.
*/

#include <stdio.h>
#include "base2.h"
#include "parse.h"
#include "pipe.h"
#include "xboard.h"
#include "pregen.h"
#include "search.h"

/* UCCIָ�����ģ��������UCCIָ���������ɡ�
 *
 * ���е�һ��������"BootLine()"��򵥣�ֻ������������������ĵ�һ��ָ��
 * ����"ucci"ʱ�ͷ���"UCCI_COMM_UCCI"������һ�ɷ���"UCCI_COMM_UNKNOWN"
 * ǰ�������������ȴ��Ƿ������룬���û��������ִ�д���ָ��"Idle()"
 * ��������������("BusyLine()"��ֻ��������˼��ʱ)����û������ʱֱ�ӷ���"UCCI_COMM_UNKNOWN"
 */
static PipeStruct pipeStd;

//const int MAX_MOVE_NUM = 256;

static char szFen[LINE_INPUT_MAX_CHAR];
static uint32_t dwCoordList[MAX_MOVE_NUM];

int xb_mv_left;
int xb_mv_ctrl;
int xb_tm_left;
int xb_tm_ctrl;
int xb_tm_incr;
int xb_post;
int xb_ponder;
int xb_depth;
int xb_force;
int hint_mv;
// FILE *myfp;

static bool ParsePos(UcciCommStruct &UcciComm, char *lp) {
  int i;
  // // �����ж��Ƿ�ָ����FEN��
  // if (StrEqvSkip(lp, "fen ")) {
    strcpy(szFen, lp);
    UcciComm.szFenStr = szFen;
  // // Ȼ���ж��Ƿ���startpos
  // } else if (StrEqv(lp, "startpos")) {
  //   UcciComm.szFenStr = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";
  // // ������߶����ǣ�����������
  // } else {
  //   return false;
  // }
  // // Ȼ��Ѱ���Ƿ�ָ���˺����ŷ������Ƿ���"moves"�ؼ���
  UcciComm.nMoveNum = 0;
  // if (StrScanSkip(lp, " moves ")) {
  //   *(lp - strlen(" moves ")) = '\0';
  //   UcciComm.nMoveNum = MIN((int) (strlen(lp) + 1) / 5, MAX_MOVE_NUM); // ��ʾ��"moves"�����ÿ���ŷ�����1���ո��4���ַ�
  //   for (i = 0; i < UcciComm.nMoveNum; i ++) {
  //     dwCoordList[i] = *(uint32_t *) lp; // 4���ַ���ת��Ϊһ��"uint32_t"���洢�ʹ�����������
  //     lp += sizeof(uint32_t) + 1;
  //   }
  //   UcciComm.lpdwMovesCoord = dwCoordList;
  // }
  return true;
}

UcciCommEnum BootLine(void) {
  char szLineStr[LINE_INPUT_MAX_CHAR];
  pipeStd.Open();
  while (!pipeStd.LineInput(szLineStr)) {
    Idle();
  }
  if (StrEqv(szLineStr, "xboard")) {
    return UCCI_COMM_XBOARD;
  } else {
    return UCCI_COMM_UNKNOWN;
  }
}

UcciCommEnum IdleLine(UcciCommStruct &UcciComm, bool bDebug) {
  char szLineStr[LINE_INPUT_MAX_CHAR];
  char fen[1024];
  char *lp;
  int i;
  bool bGoTime;

  while (!pipeStd.LineInput(szLineStr)) {
    Idle();
  }

  // fputs(szLineStr, myfp);
  // fputs("\n", myfp);

  lp = szLineStr;
  if (bDebug) {
    printf("info idleline [%s]\n", lp);
    fflush(stdout);
  }
  if (false) {
  // "IdleLine()"����ӵ�UCCIָ����������������UCCIָ����������ͣ�������

  // 1. protover
  } else if (StrEqv(lp, "protover ")) {
      puts  ("feature done=0");
      puts  ("feature myname=\"(elephant eye for xboard)\"");
      puts  ("feature variants=\"xiangqi\"");
      puts  ("feature setboard=1");
      puts  ("feature debug=1");
      puts  ("feature sigint=0");
      puts  ("feature sigterm=0");
      puts  ("feature getboard=1");
      puts  ("feature colors=0");
      puts  ("feature usermove=1");
      printf("feature option=\"Promotion -check %d\"\n", PreEval.bPromotion?1:0);
      printf("feature option=\"Perpetual Check -check %d\"\n", Search.bAlwaysCheck);
      printf("feature option=\"Use Hash -check %d\"\n", Search.bUseHash);
      printf("feature option=\"Use Book -check %d\"\n", Search.bUseBook);
      printf("feature option=\"Book File -file %s\"\n", Search.szBookFile);
      printf("feature option=\"Libeval File -file %s\"\n", Search.szLibEvalFile);
      printf("feature option=\"Hash Size Exp -spin %d 4 10\"\n", Search.nHashSize);
      printf("feature option=\"Prune -check %d\"\n", Search.bNullMove);
      printf("feature option=\"Position Knowledge -check %d\"\n", Search.bKnowledge);
      printf("feature option=\"Randomness -spin %d 0 3\"\n", Search.nRandom);
      puts  ("feature done=1");
      return UCCI_COMM_UNKNOWN;

  // 2. option
  } else if (StrEqvSkip(lp, "option ")) {
      if (false) {}
      else if (StrEqvSkip(lp, "Promotion=")) {
          UcciComm.Option = UCCI_OPTION_PROMOTION;
          UcciComm.bCheck = StrEqv(lp, "0")?false:true;
      }
      else if (StrEqvSkip(lp, "Perpetual Check=")) {
          UcciComm.Option = UCCI_OPTION_ALWAYSCHECK;
          UcciComm.bCheck = StrEqv(lp, "0")?false:true;
      }
      else if (StrEqvSkip(lp, "Use Hash=")) {
          UcciComm.Option = UCCI_OPTION_USEHASH;
          UcciComm.bCheck = StrEqv(lp, "0")?false:true;
      }
      else if (StrEqvSkip(lp, "Use Book=")) {
          UcciComm.Option = UCCI_OPTION_USEBOOK;
          UcciComm.bCheck = StrEqv(lp, "0")?false:true;
      }
      else if (StrEqvSkip(lp, "Book File=")) {
          UcciComm.Option = UCCI_OPTION_BOOKFILES;
          UcciComm.szOption = lp;
      }
      else if (StrEqvSkip(lp, "Libeval File=")) {
          UcciComm.Option = UCCI_OPTION_EVALAPI;
          UcciComm.szOption = lp;
      }
      else if (StrEqvSkip(lp, "Hash Size Exp=")) {
          UcciComm.Option = UCCI_OPTION_HASHSIZE;
          UcciComm.nSpin = Str2Digit(lp, 4, 10);
      }
      else if (StrEqvSkip(lp, "Prune=")) {
          UcciComm.Option = UCCI_OPTION_PRUNING;
          UcciComm.bCheck = StrEqv(lp, "0")?false:true;
      }
      else if (StrEqvSkip(lp, "Position Knowledge=")) {
          UcciComm.Option = UCCI_OPTION_KNOWLEDGE;
          UcciComm.bCheck = StrEqv(lp, "0")?false:true;
      }
      else if (StrEqvSkip(lp, "Randomness=")) {
          UcciComm.Option = UCCI_OPTION_RANDOMNESS;
          UcciComm.nSpin = Str2Digit(lp, 0, 3);
      }
      return UCCI_COMM_SETOPTION;

  // setboard
  } else if (StrEqvSkip(lp, "setboard ")) {
    return ParsePos(UcciComm, lp) ? UCCI_COMM_POSITION : UCCI_COMM_UNKNOWN;

  // 4. "banmoves <move_list>"ָ�����������"position ... moves ..."��һ����
  } else if (StrEqvSkip(lp, "banmoves ")) {
    UcciComm.nBanMoveNum = MIN((int) (strlen(lp) + 1) / 5, MAX_MOVE_NUM);
    for (i = 0; i < UcciComm.nBanMoveNum; i ++) {
      dwCoordList[i] = *(uint32_t *) lp;
      lp += sizeof(uint32_t) + 1;
    }
    UcciComm.lpdwBanMovesCoord = dwCoordList;
    return UCCI_COMM_BANMOVES;
  }
  /* go */
  else if (StrEqv(lp, "go")) {
    // UcciComm.bPonder = UcciComm.bDraw = false;
    // // �����жϵ�����"go"��"go ponder"����"go draw"
    // if (StrEqvSkip(lp, "ponder ")) {
    //   UcciComm.bPonder = true;
    // } else if (StrEqvSkip(lp, "draw ")) {
    //   UcciComm.bDraw = true;
    // }
    // // Ȼ���ж�˼��ģʽ
    // bGoTime = false;
    // if (false) {
    // } else if (StrEqvSkip(lp, "depth ")) {
    //   UcciComm.Go = UCCI_GO_DEPTH;
    //   UcciComm.nDepth = Str2Digit(lp, 0, UCCI_MAX_DEPTH);
    // } else if (StrEqvSkip(lp, "nodes ")) {
    //   UcciComm.Go = UCCI_GO_NODES;
    //   UcciComm.nDepth = Str2Digit(lp, 0, 2000000000);
    // } else if (StrEqvSkip(lp, "time ")) {
    //   UcciComm.nTime = Str2Digit(lp, 0, 2000000000);
    //   bGoTime = true;
    // // ���û��˵���ǹ̶���Ȼ����趨ʱ�ޣ��͹̶����Ϊ"UCCI_MAX_DEPTH"
    // } else {
    //   UcciComm.Go = UCCI_GO_DEPTH;
    //   UcciComm.nDepth = UCCI_MAX_DEPTH;
    // }
    // // ������趨ʱ�ޣ���Ҫ�ж���ʱ���ƻ��Ǽ�ʱ��
    // if (bGoTime) {
    //   if (false) {
    //   } else if (StrScanSkip(lp, " movestogo ")) {
    //     UcciComm.Go = UCCI_GO_TIME_MOVESTOGO;
    //     UcciComm.nMovesToGo = Str2Digit(lp, 1, 999);
    //   } else if (StrScanSkip(lp, " increment ")) {
    //     UcciComm.Go = UCCI_GO_TIME_INCREMENT;
    //     UcciComm.nIncrement = Str2Digit(lp, 0, 999999);
    //   // ���û��˵����ʱ���ƻ��Ǽ�ʱ�ƣ����趨Ϊ������1��ʱ����
    //   } else {
    //     UcciComm.Go = UCCI_GO_TIME_MOVESTOGO;
    //     UcciComm.nMovesToGo = 1;
    //   }
    // }
    return UCCI_COMM_GO;

  // 6. "stop"ָ��
  } else if (StrEqv(lp, "stop")) {
    return UCCI_COMM_STOP;

  // 7. "probe {<special_position> | fen <fen_string>} [moves <move_list>]"ָ��
  } else if (StrEqvSkip(lp, "probe ")) {
    return ParsePos(UcciComm, lp) ? UCCI_COMM_PROBE : UCCI_COMM_UNKNOWN;

  // 8. "quit"ָ��
  } else if (StrEqv(lp, "quit")) {
    return UCCI_COMM_QUIT;
  }
  /* usermove */
  else if (StrEqvSkip(lp, "usermove ")) {
      UcciComm.dwMoveCoord = *(uint32_t *)lp;
      return UCCI_COMM_USERMOVE;
  }
  /* getboard */
  else if (StrEqv(lp, "getboard")) {
      Search.pos.ToFen(fen);
      puts(fen);
      return UCCI_COMM_UNKNOWN;
  }
  /* time */
  else if (StrEqvSkip(lp, "time ")) {
      xb_tm_left = MAX(0, atoi(lp));
      return UCCI_COMM_UNKNOWN;
  }
  /* force */
  else if (StrEqv(lp, "force")) {
      xb_force = 1;
      return UCCI_COMM_UNKNOWN;
  }
  /* post */
  else if (StrEqv(lp, "post")) {
      xb_post = 1;
      return UCCI_COMM_UNKNOWN;
  }
  /* nopost */
  else if (StrEqv(lp, "nopost")) {
      xb_post = 0;
      return UCCI_COMM_UNKNOWN;
  }
  /* hard */
  else if (StrEqv(lp, "hard")) {
      xb_ponder = 1;
      return UCCI_COMM_UNKNOWN;
  }
  /* easy */
  else if (StrEqv(lp, "easy")) {
      xb_ponder = 0;
      return UCCI_COMM_UNKNOWN;
  }
  /* sd */
  else if (StrEqvSkip(lp, "sd ")) {
      i = atoi(lp);
      xb_depth = MAX(1, MIN(i, UCCI_MAX_DEPTH));
      return UCCI_COMM_UNKNOWN;
  }

  else if (StrEqvSkip(lp, "st ")) {
      xb_mv_left = xb_mv_ctrl = 1;
      xb_tm_left = xb_tm_ctrl = atoi(lp);
      xb_tm_incr = 0;
      return UCCI_COMM_UNKNOWN;
  }
  /* level */
  else if (StrEqvSkip(lp, "level ")) {
      if (3 == sscanf(lp, "%d %d %d",
                      &xb_mv_ctrl,
                      &xb_tm_ctrl,
                      &xb_tm_incr) ||
          4 == sscanf(lp, "%d %d:%d %d",
                      &xb_mv_ctrl,
                      &xb_tm_ctrl,
                      &i,
                      &xb_tm_incr)) {
          xb_mv_left = xb_mv_ctrl;
          /* in 1/100 s */
          xb_tm_ctrl = xb_tm_left = 6000 * xb_tm_ctrl + 100 * i;
      }      
      return UCCI_COMM_UNKNOWN;
  }
  /* random */
  else if (StrEqv(lp, "random")) {
      if (Search.nRandom) {
          Search.nRandomSave = Search.nRandom;
          Search.nRandom = 0;
      }
      else {
          Search.nRandom = Search.nRandomSave;
      }
      Search.nRandomMask = (1 << Search.nRandom) - 1;
      return UCCI_COMM_UNKNOWN;
  }
  /* new */
  else if (StrEqv(lp, "new")) {
      return UCCI_COMM_NEW;
  }
  /* hint */
  else if (StrEqv(lp, "hint")) {
      uint32_t coord;
      if (hint_mv) {
          coord = MOVE_COORD(hint_mv);
          printf("Hint: %.4s\n", (char *)&coord);
      }
      return UCCI_COMM_UNKNOWN;
  }
  else if (StrEqv(lp, "undo")) {
      if (xb_force)
          return UCCI_COMM_UNDO;
      printf("Error (command not legal now): %s\n", lp);
      return UCCI_COMM_UNKNOWN;
  }
  else if (StrEqv(lp, "remove")) {
      return UCCI_COMM_REMOVE;
  }
  else if (StrEqv(lp, "playother")) {
      puts("Error (not implemented): playother");
      return UCCI_COMM_UNKNOWN;
  }
  else {
    return UCCI_COMM_UNKNOWN;
  }
}

UcciCommEnum BusyLine(UcciCommStruct &UcciComm, bool bDebug) {
  char szLineStr[LINE_INPUT_MAX_CHAR];
  char *lp;
  if (pipeStd.LineInput(szLineStr)) {
    if (bDebug) {
      printf("info busyline [%s]\n", szLineStr);
      fflush(stdout);
    }
    // "BusyLine"ֻ�ܽ���"isready"��"ponderhit"��"stop"������ָ��
    if (false) {
    } else if (StrEqv(szLineStr, "protover ")) {
        puts  ("feature done=0");
        puts  ("feature myname=\"(elephant eye for xboard)\"");
        puts  ("feature variants=\"xiangqi\"");
        puts  ("feature setboard=1");
        puts  ("feature debug=1");
        puts  ("feature sigint=0");
        puts  ("feature sigterm=0");
        puts  ("feature getboard=1");
        puts  ("feature colors=0");
        puts  ("feature usermove=1");
        printf("feature option=\"Promotion -check %d\"\n", PreEval.bPromotion?1:0);
        printf("feature option=\"Perpetual Check -check %d\"\n", Search.bAlwaysCheck);
        printf("feature option=\"Use Hash -check %d\"\n", Search.bUseHash);
        printf("feature option=\"Use Book -check %d\"\n", Search.bUseBook);
        printf("feature option=\"Book File -file %s\"\n", Search.szBookFile);
        printf("feature option=\"Libeval File -file %s\"\n", Search.szLibEvalFile);
        printf("feature option=\"Hash Size Exp -spin %d 4 10\"\n", Search.nHashSize);
        printf("feature option=\"Prune -check %d\"\n", Search.bNullMove);
        printf("feature option=\"Position Knowledge -check %d\"\n", Search.bKnowledge);
        printf("feature option=\"Randomness -spin %d 0 3\"\n", Search.nRandom);
        puts  ("feature done=1");
        return UCCI_COMM_UNKNOWN;
    } else if (StrEqv(szLineStr, "ponderhit draw")) {
      return UCCI_COMM_PONDERHIT_DRAW;
    // ע�⣺���������ж�"ponderhit draw"�����ж�"ponderhit"
    } else if (StrEqv(szLineStr, "ponderhit")) {
      return UCCI_COMM_PONDERHIT;
    } else if (StrEqv(szLineStr, "?")) {
      return UCCI_COMM_STOP;
    } else if (StrEqv(szLineStr, "quit")) {
      return UCCI_COMM_QUIT;
    } else {
        lp = szLineStr;
        if (StrEqvSkip(lp, "usermove")) {
            UcciComm.dwMoveCoord = *(uint32_t *)lp;
            return UCCI_COMM_USERMOVE;
        }
        return UCCI_COMM_UNKNOWN;
    }
  } else {
    return UCCI_COMM_UNKNOWN;
  }
}
