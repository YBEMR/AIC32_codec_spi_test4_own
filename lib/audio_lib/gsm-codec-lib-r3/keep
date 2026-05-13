SUBDIR_LIBPROD=	libgsmefr libgsmfr2 libtwamr
SUBDIR_UTILS=	amrconv amrefr amrtest efrtest frtest miscutil
SUBDIR_INT=	dev libtest

SUBDIR=	${SUBDIR_LIBPROD} ${SUBDIR_UTILS} ${SUBDIR_INT}
SUBDIR_INST=	${SUBDIR_LIBPROD} ${SUBDIR_UTILS}
DESTDIR=

all:	${SUBDIR}

amrconv:	libtest
amrefr:		libgsmefr libtest libtwamr
amrtest:	libtest libtwamr
efrtest:	libgsmefr libtest
frtest:		libgsmfr2 libtest
miscutil:	libgsmefr libgsmfr2 libtest

${SUBDIR}: FRC config.defs
	cd $@; ${MAKE} ${MFLAGS}

config.defs:
	@echo 'You must run ./configure before make'
	@false

install:	FRC
	for i in ${SUBDIR_INST}; do (cd $$i; ${MAKE} ${MFLAGS} \
		DESTDIR=${DESTDIR} install); done

# The following two subset install targets are only for experts;
# if you are not sure why they are needed, you *don't* need them!

install-lib:	FRC
	for i in ${SUBDIR_LIBPROD}; do (cd $$i; ${MAKE} ${MFLAGS} \
		DESTDIR=${DESTDIR} install); done

install-utils:	FRC
	for i in ${SUBDIR_UTILS}; do (cd $$i; ${MAKE} ${MFLAGS} \
		DESTDIR=${DESTDIR} install); done

clean: FRC
	rm -f a.out core errs
	for i in ${SUBDIR}; do (cd $$i; ${MAKE} ${MFLAGS} clean); done

FRC:
