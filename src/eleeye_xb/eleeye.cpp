/*
eleeye.cpp - Source Code for ElephantEye, Part IX

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.15, Last Modified: Jul. 2008
Copyright (C) 2004-2008 www.elephantbase.net

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include "base2.h"
#include "parse.h"
#include "xboard.h"
#include "pregen.h"
#include "position.h"
#include "hash.h"
#include "search.h"
#include "cchess.h"
#include "evaluate.h"
#include "preeval.h"

#ifdef _WIN32
  #include <windows.h>
  const char *const cszLibEvalFile = "";
#else
  #include <dlfcn.h>
  #define WINAPI
const char *const cszLibEvalFile = "";
#endif

const int INTERRUPT_COUNT = 4096; // 搜索若干结点后调用中断

// static const char *WINAPI GetEngineName(void) {
//   return NULL;
// }

// static void WINAPI PreEvaluate(PositionStruct *lppos, PreEvalStruct *lpPreEval) {
//   // 缺省的局面预评价过程，什么都不做
// }

// static int WINAPI Evaluate(const PositionStruct *lppos, int vlAlpha, int vlBeta) {
//   // 缺省的局面评价过程，只返回子力价值
//   return lppos->Material();
// }

#ifdef _WIN32

inline HMODULE LoadEvalApi(const char *szLibEvalFile) {
  HMODULE hModule;  
  hModule = LoadLibrary(szLibEvalFile);
  if (hModule == NULL) {
    Search.GetEngineName = GetEngineName;
    Search.PreEvaluate = PreEvaluate;
    Search.Evaluate = Evaluate;
  } else {
    Search.GetEngineName = (const char *(WINAPI *)(void)) GetProcAddress(hModule, "_GetEngineName@0");
    Search.PreEvaluate = (void (WINAPI *)(PositionStruct *, PreEvalStruct *)) GetProcAddress(hModule, "_PreEvaluate@8");
    Search.Evaluate = (int (WINAPI *)(const PositionStruct *, int, int)) GetProcAddress(hModule, "_Evaluate@12");
  }
  return hModule;
}

inline void FreeEvalApi(HMODULE hModule) {
  if (hModule != NULL) {
    FreeLibrary(hModule);
  }
}

#else

inline void *LoadEvalApi(const char *szLibEvalFile) {
  void *hModule;
  hModule = dlopen(szLibEvalFile, RTLD_LAZY);
  if (hModule == NULL) {
    Search.GetEngineName = GetEngineName;
    Search.PreEvaluate = PreEvaluate;
    Search.Evaluate = Evaluate;
  } else {
    Search.GetEngineName = (const char *(*)(void)) dlsym(hModule, "GetEngineName");
    Search.PreEvaluate = (void (*)(PositionStruct *, PreEvalStruct *)) dlsym(hModule, "PreEvaluate");
    Search.Evaluate = (int (*)(const PositionStruct *, int, int)) dlsym(hModule, "Evaluate");
  }
  return hModule;
}

inline void FreeEvalApi(void *hModule) {
  if (hModule != NULL) {
    dlclose(hModule);
  }
}

#endif

inline void PrintLn(const char *sz) {
  printf("%s\n", sz);
  fflush(stdout);
}

static void prepare_clock(void)
{
    Search.nGoMode = GO_MODE_TIMER;
    if (xb_tm_incr == 0 && xb_tm_left != 0) {
        Search.nProperTimer = xb_tm_left * 10 / xb_mv_left;
        Search.nMaxTimer = xb_tm_left * MAX(5, 11 - xb_mv_left);
    } else {
        Search.nProperTimer = (xb_tm_left / 20 + xb_tm_incr) * 10;
        Search.nMaxTimer = xb_tm_left * 5;
    }
    Search.nProperTimer += (xb_ponder ? Search.nProperTimer / 4 : 0);
    Search.nMaxTimer = MIN(Search.nMaxTimer, Search.nProperTimer * 10);
}

static void update_clock(void)
{
    xb_tm_left -= Search.nTimeUsed / 10 - xb_tm_incr;
    xb_mv_left -= 1;
    if (!xb_mv_left) {
        xb_tm_left += xb_tm_ctrl;
        xb_mv_left = xb_mv_ctrl;
    }
}

int main(void) {
  int i;
  bool bPonderTime;
  UcciCommStruct UcciComm;
  const char *szEngineName;
  PositionStruct posProbe;
#ifdef _WIN32
  HMODULE hModule;  
#else
  void *hModule;
#endif

  int mv, stat;
  char fen[1024];

  /* disable stdio buffering */
  if (setvbuf(stdin, NULL, _IONBF, 0) != 0)
      perror("setvbuf");
  if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
      perror("setvbuf");

  // myfp = fopen("test", "w+");
  // fputs("test\n", myfp);

  xb_mv_left = 40;
  xb_mv_ctrl = 40;
  xb_tm_left = 30000;
  xb_tm_ctrl = 30000;
  xb_tm_incr = 0;
  xb_post = 0;
  xb_ponder = 0;
  xb_depth = UCCI_MAX_DEPTH;
  xb_force = 0;
  Search.nHashSize = 4;
  Search.nRandom = 0;
  Search.nRandomSave = 2;

  /* if not working with autotools */
#ifndef PKGDATADIR
#define PKGDATADIR "."
#endif

  LocatePath(Search.szBookFile, PKGDATADIR "/book.dat");
  //LocatePath(Search.szBookFile, "book.dat");
  LocatePath(Search.szLibEvalFile, cszLibEvalFile);
  hModule = LoadEvalApi(Search.szLibEvalFile);

  bPonderTime = false;
  PreGenInit();
  NewHash(24); // 24=16MB, 25=32MB, 26=64MB, ...
  Search.pos.FromFen(cszStartFen);
  Search.pos.nDistance = 0;
  Search.PreEvaluate(&Search.pos, &PreEval);
  Search.nBanMoves = 0;
  Search.bQuit = Search.bBatch = Search.bDebug = Search.bAlwaysCheck = false;
  Search.bUseHash = Search.bUseBook = Search.bNullMove = Search.bKnowledge = true;
  Search.bIdle = false;
  Search.nCountMask = INTERRUPT_COUNT - 1;
  Search.nRandomMask = 0;
  Search.rc4Random.InitRand();

  szEngineName = Search.GetEngineName();
  if (szEngineName == NULL) {
    PrintLn("id name ElephantEye");
  } else {
    printf("id name %s / ElephantEye\n", szEngineName);
    fflush(stdout);
  }
  PrintLn("id version 3.15");
  PrintLn("id copyright 2004-2008 www.elephantbase.net");
  PrintLn("id author Morning Yellow");
  PrintLn("id user ElephantEye Test Team");

  while (BootLine() != UCCI_COMM_XBOARD);
  // PrintLn("option usemillisec type check default true");
  // PrintLn("option promotion type check default false");
  // PrintLn("option batch type check default false");
  // PrintLn("option debug type check default false");
  // PrintLn("option ponder type check default false");
  // PrintLn("option alwayscheck type check default false");
  // PrintLn("option usehash type check default true");
  // PrintLn("option usebook type check default true");
  // printf("option bookfiles type string default %s\n", Search.szBookFile);
  // fflush(stdout);
  // printf("option evalapi type string default %s\n", szLibEvalFile);
  // fflush(stdout);
  // PrintLn("option hashsize type spin min 16 max 1024 default 16");
  // PrintLn("option idle type combo var none var small var medium var large default none");
  // PrintLn("option pruning type combo var none var small var medium var large default large");
  // PrintLn("option knowledge type combo var none var small var medium var large default large");
  // PrintLn("option randomness type combo var none var small var medium var large default none");
  // PrintLn("option newgame type button");
  // PrintLn("ucciok");

  // 以下是接收指令和提供对策的循环体
  while (!Search.bQuit) {
    switch (IdleLine(UcciComm, Search.bDebug)) {
    case UCCI_COMM_STOP:
      PrintLn("nobestmove");
      break;
    case UCCI_COMM_POSITION:
      BuildPos(Search.pos, UcciComm);
      Search.pos.nDistance = 0;
      Search.PreEvaluate(&Search.pos, &PreEval);
      Search.nBanMoves = 0;
      hint_mv = 0;
      break;
    case UCCI_COMM_NEW:
        Search.pos.FromFen(cszStartFen);
        Search.pos.nDistance = 0;
        Search.PreEvaluate(&Search.pos, &PreEval);
        Search.nBanMoves = 0;
        xb_force = 0;
        xb_tm_left = xb_tm_ctrl;
        xb_mv_left = xb_mv_ctrl;
        xb_depth = UCCI_MAX_DEPTH;
        hint_mv = 0;
        break;
    case UCCI_COMM_BANMOVES:
      Search.nBanMoves = UcciComm.nBanMoveNum;
      for (i = 0; i < UcciComm.nBanMoveNum; i ++) {
        Search.wmvBanList[i] = COORD_MOVE(UcciComm.lpdwBanMovesCoord[i]);
      }
      break;
    case UCCI_COMM_SETOPTION:
      switch (UcciComm.Option) {
      case UCCI_OPTION_PROMOTION:
        PreEval.bPromotion = UcciComm.bCheck;
        break;
      case UCCI_OPTION_ALWAYSCHECK:
        Search.bAlwaysCheck = UcciComm.bCheck;
        break;
      case UCCI_OPTION_USEHASH:
        Search.bUseHash = UcciComm.bCheck;
        break;
      case UCCI_OPTION_USEBOOK:
        Search.bUseBook = UcciComm.bCheck;
        break;
      case UCCI_OPTION_BOOKFILES:
        if (AbsolutePath(UcciComm.szOption)) {
          strcpy(Search.szBookFile, UcciComm.szOption);
        } else {
          LocatePath(Search.szBookFile, UcciComm.szOption);
        }
        break;
      case UCCI_OPTION_EVALAPI:
        if (AbsolutePath(UcciComm.szOption)) {
          strcpy(Search.szLibEvalFile, UcciComm.szOption);
        } else {
          LocatePath(Search.szLibEvalFile, UcciComm.szOption);
        }
        FreeEvalApi(hModule);
        hModule = LoadEvalApi(Search.szLibEvalFile);
        break;
      case UCCI_OPTION_HASHSIZE:
        DelHash();
        Search.nHashSize = UcciComm.nSpin;
        NewHash(UcciComm.nSpin + 20);
        break;
      case UCCI_OPTION_PRUNING:
          Search.bNullMove = UcciComm.bCheck;
          break;
      case UCCI_OPTION_KNOWLEDGE:
          Search.bKnowledge = UcciComm.bCheck;
          break;
      case UCCI_OPTION_RANDOMNESS:
          Search.nRandom = UcciComm.nSpin;
          if (UcciComm.nSpin)
              Search.nRandomSave = UcciComm.nSpin;
          Search.nRandomMask = (1 << Search.nRandom) - 1;
          break;
      default:
          break;
      }
      break;
    case UCCI_COMM_GO:
      // Search.bPonder = UcciComm.bPonder;
      // Search.bDraw = UcciComm.bDraw;
      // switch (UcciComm.Go) {
      // case UCCI_GO_DEPTH:
      //   Search.nGoMode = GO_MODE_INFINITY;
      //   Search.nNodes = 0;
      //   SearchMain(UcciComm.nDepth);
      //   break;
      // case UCCI_GO_NODES:
      //   Search.nGoMode = GO_MODE_NODES;
      //   Search.nNodes = UcciComm.nNodes;
      //   SearchMain(UCCI_MAX_DEPTH);
      //   break;
      // case UCCI_GO_TIME_MOVESTOGO:
      // case UCCI_GO_TIME_INCREMENT:
      //   Search.nGoMode = GO_MODE_TIMER;
      //   if (UcciComm.Go == UCCI_GO_TIME_MOVESTOGO) {
      //     // 对于时段制，把剩余时间平均分配到每一步，作为适当时限。
      //     // 剩余步数从1到5，最大时限依次是剩余时间的100%、90%、80%、70%和60%，5以上都是50%
      //     Search.nProperTimer = UcciComm.nTime / UcciComm.nMovesToGo;
      //     Search.nMaxTimer = UcciComm.nTime * MAX(5, 11 - UcciComm.nMovesToGo) / 10;
      //   } else {
      //     // 对于加时制，假设棋局会在20回合内结束，算出平均每一步的适当时限，最大时限是剩余时间的一半
      //     Search.nProperTimer = UcciComm.nTime / 20 + UcciComm.nIncrement;
      //     Search.nMaxTimer = UcciComm.nTime / 2;
      //   }
      //   // 如果是后台思考的时间分配策略，那么适当时限设为原来的1.25倍
      //   Search.nProperTimer += (bPonderTime ? Search.nProperTimer / 4 : 0);
      //   Search.nMaxTimer = MIN(Search.nMaxTimer, Search.nProperTimer * 10);
      //   SearchMain(UCCI_MAX_DEPTH);
      //   break;
      // default:
      //   break;
      // }
        xb_force = 0;
        Search.nGoMode = GO_MODE_TIMER;
        prepare_clock();
        SearchMain(xb_depth);
        update_clock();
        break;
    case UCCI_COMM_PROBE:
      BuildPos(posProbe, UcciComm);
      if (!PopHash(posProbe)) {
        PopLeaf(posProbe);
      }
      break;
    case UCCI_COMM_QUIT:
      Search.bQuit = true;
      break;
    case UCCI_COMM_UNDO:
        if (Search.pos.nDistance >= 1)
            Search.pos.UndoMakeMove();
        hint_mv = 0;
        break;
    case UCCI_COMM_REMOVE:
        if (Search.pos.nDistance >= 2) {
            Search.pos.UndoMakeMove();
            Search.pos.UndoMakeMove();
        }
        hint_mv = 0;
        break;
    case UCCI_COMM_USERMOVE:
        mv = COORD_MOVE(UcciComm.dwMoveCoord);
        TryMove(Search.pos, stat, mv);
        switch (stat) {
        case MOVE_ILLEGAL:
            printf("Illegal move: %.4s\n", (char *)&UcciComm.dwMoveCoord);
            break;
        case MOVE_INCHECK:
            printf("Illegal move (in check): %.4s\n", (char *)&UcciComm.dwMoveCoord);
            break;
        case MOVE_DRAW:
            puts("1/2-1/2 {Draw by rule}\n");
            break;
        case MOVE_PERPETUAL_LOSS:
            printf("Illegal move (perpetual attack): %.4s\n", (char *)&UcciComm.dwMoveCoord);
            break;
        case MOVE_PERPETUAL:
            puts("1/2-1/2 {Draw by repetition}\n");
            break;
        case MOVE_MATE:
            if (Search.pos.sdPlayer == 0)
                puts("1-0 {checkmate or stalemate}\n");
            else
                puts("0-1 {checkmate or stalemate}\n");
            break;
        default:
            Search.pos.MakeMove(mv);
            if (!xb_force) {
                prepare_clock();
                SearchMain(xb_depth);
                update_clock();
            }
            break;
        }
        break;
    default:
      break;
    }
  }
  DelHash();
  FreeEvalApi(hModule);
  return 0;
}
