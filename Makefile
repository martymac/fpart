# Makefile for fpart(1) - http://contribs.martymac.org
#
# Copyright (c) 2011 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

# Compiler options
CC?= cc
CFLAGS+= -Wall -std=c99

# GNU/Linux users will have to use the following to avoid warnings
#CFLAGS+= -Wall -std=c99 -D_GNU_SOURCE

# Use the following to build with debug symbols and messages
#CFLAGS+= -Wall -g -DDEBUG -std=c99

# Configuration / variables section
DESTDIR= 
PREFIX?= /usr/local

# Default installation paths
BINDIR= $(PREFIX)/bin
MANDIR= $(PREFIX)/man

# Binaries
INSTALL_PROGRAM?=	install -s -m 555
INSTALL_DIR?=	install -d -m 755
RM?= rm
CAT?= cat
GZIP?= gzip

all: fpart

utils.o: utils.c utils.h
	${CC} ${CFLAGS} -c utils.c

options.o: options.c options.h
	${CC} ${CFLAGS} -c options.c

partition.o: partition.c partition.h
	${CC} ${CFLAGS} -c partition.c

file_entry.o: file_entry.c file_entry.h
	${CC} ${CFLAGS} -c file_entry.c

dispatch.o: dispatch.c dispatch.h
	${CC} ${CFLAGS} -c dispatch.c

fpart: utils.o options.o partition.o file_entry.o dispatch.o fpart.c fpart.h
	${CC} ${CFLAGS} -lm utils.o options.o partition.o file_entry.o dispatch.o \
		fpart.c -o fpart

install: installbin installman

installbin: fpart
	@echo -n 'Installing fpart into $(DESTDIR)$(BINDIR)... '
	@$(INSTALL_DIR) '$(DESTDIR)$(BINDIR)' 2>/dev/null
	@$(INSTALL_PROGRAM) fpart '$(DESTDIR)$(BINDIR)/fpart'
	@echo 'ok.'

installman: fpart.1
	@echo -n 'Installing fpart(1) man page into $(DESTDIR)$(MANDIR)... '
	@$(INSTALL_DIR) '$(DESTDIR)$(MANDIR)/man1' 2>/dev/null
	@$(CAT) fpart.1 | $(GZIP) - > '$(DESTDIR)$(MANDIR)/man1/fpart.1.gz'
	@echo 'ok.'

uninstall: uninstallbin uninstallman

uninstallbin:
	@echo -n 'Removing fpart from $(DESTDIR)$(BINDIR)... '
	@$(RM) -f '$(DESTDIR)$(BINDIR)/fpart'
	@echo 'ok.'

uninstallman:
	@echo -n 'Removing fpart(1) man page from $(DESTDIR)$(MANDIR)... '
	@$(RM) -f '$(DESTDIR)$(MANDIR)/man1/fpart.1.gz'
	@echo 'ok.'

clean:
	${RM} -f fpart *.o *.core
