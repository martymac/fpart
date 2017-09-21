# Disable -I.
AUTOMAKE_OPTIONS = nostdinc

bin_PROGRAMS = fpart
fpart_SOURCES = types.h utils.c utils.h options.c options.h partition.c partition.h file_entry.c file_entry.h dispatch.c dispatch.h fpart.c fpart.h
fpart_CFLAGS =
fpart_LDFLAGS =

if DEBUG
fpart_CFLAGS += -g -DDEBUG
endif

if EMBEDDED_FTS
fpart_SOURCES += fts.c fts.h
fpart_CFLAGS += -DEMBED_FTS
endif

if SOLARIS
fpart_CFLAGS += -D_POSIX_C_SOURCE=200112L -D__EXTENSIONS__ -xc99
endif

if LINUX
fpart_CFLAGS += -D_GNU_SOURCE
endif

if STATIC
fpart_LDFLAGS += -static
endif
