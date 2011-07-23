INCLUDES="."
LINKDIRS=
LINKLIBS=-lncurses -llept -lm

MAINFILE=pato
OUTFILE=${MAINFILE}.out


FILES=`rcb ${MAINFILE}.c`

CFLAGS_OWN=-Wall -D_GNU_SOURCE
CFLAGS_DBG=-g
CFLAGS_OPT=-s -Os

-include config.mak

all: debug

optimized: 
	${CC} ${CFLAGS_OWN} ${CFLAGS_OPT} -I ${INCLUDES} ${FILES} ${LINKLIBS} ${CFLAGS} -o ${OUTFILE}

debug: 
	${CC} ${CFLAGS_OWN} ${CFLAGS_DBG} -I ${INCLUDES} ${FILES} ${LINKLIBS} ${CFLAGS} -o ${OUTFILE}


.PHONY: all optimized debug
