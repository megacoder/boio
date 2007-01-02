#########################################################################
# vim: ts=4 sw=4
#########################################################################
# Author:   reynolds (Tommy Reynolds)
# Filename: Makefile
# Date:     2007-01-01 10:56:52
#########################################################################
# Note that we use '::' rules to allow multiple rule sets for the same
# target.  Read that as "modularity exemplarized".
#########################################################################

PREFIX	:=/opt/boio
BINDIR	=${PREFIX}/bin

TARGETS	=all clean distclean clobber check install uninstall tags
TARGET	=all

SUBDIRS	=

.PHONY:	${TARGETS} ${SUBDIRS}

CC		=ccache gcc -m32 -std=gnu99
INC		=-I.
OPT		=-O0
DEFS	=
CFLAGS	=${OPT} -Wall -Wextra -Werror -pedantic -g ${DEFS} ${INC}
LDFLAGS	=-g
LDLIBS	=

all::	boio

${TARGETS}::

clean::
	${RM} a.out *.o core.* lint tags

distclean clobber:: clean
	${RM} boio

check::	boio
	./boio ${ARGS}

install:: boio
	install -d ${BINDIR}
	install -c -s boio ${BINDIR}/

uninstall::
	${RM} ${BINDIR}/boio

tags::
	ctags -R .

boio.o:	boio.h boio.c

# ${TARGETS}::
# 	${MAKE} TARGET=$@ ${SUBDIRS}

# ${SUBDIRS}::
# 	${MAKE} -C $@ ${TARGET}
