OBJS=	binreader.o parse_dlcap.o pcmwrite.o roberead.o robewrite.o wavrdhelp.o\
	wavreader.o wavwriter.o
LIB=	libtest.a

include ../config.defs

all:	${LIB}

${LIB}:	${OBJS}
	ar rcu $@ ${OBJS}
	ranlib $@

clean:
	rm -f *.[oa] errs
