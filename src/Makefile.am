SUBDIRS = eleeye_xb io learn_brd

bin_PROGRAMS = robotxq catboard

robotxq_SOURCES = robotxq.c read_board.c xiangqi.c cpopen.c interface.c
catboard_SOURCES = catboard.c read_board.c xiangqi.c

robotxq_CFLAGS = -DCONFDIR="\"$(sysconfdir)\"" $(AM_CFLAGS)
catboard_CFLAGS = -DCONFDIR="\"$(sysconfdir)\"" $(AM_CFLAGS)

noinst_HEADERS = read_board.h xiangqi.h