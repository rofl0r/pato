INCLUDES="."
LINKDIRS=
LINKLIBS=-llept -lm

MAINFILE=pato.c

CFLAGS_OWN=-Wall -D_GNU_SOURCE
CFLAGS_DBG=-g
CFLAGS_OPT=-s -Os

-include config.mak

CFLAGS_RCB_OPT=${CFLAGS_OWN} ${CFLAGS_OPT} -I ${INCLUDES} ${CFLAGS}
CFLAGS_RCB_DBG=${CFLAGS_OWN} ${CFLAGS_DBG} -I ${INCLUDES} ${CFLAGS}

all: debug

clean:
	rm -f *.o
	rm -f pato.out
	rm -f *.rcb

optimized:
	CFLAGS="${CFLAGS_RCB_OPT}" rcb --force $(RCBFLAGS) ${MAINFILE} $(LINKLIBS)

debug:
	CFLAGS="${CFLAGS_RCB_DBG}" rcb --force $(RCBFLAGS) ${MAINFILE} $(LINKLIBS)


.PHONY: all optimized debug
