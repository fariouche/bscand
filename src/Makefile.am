bin_PROGRAMS = bscand

AM_CFLAGS = $(LIBCONFIG_CFLAGS) -D_GNU_SOURCE

bscand_SOURCES = main.c process.c process.h stiff.c stiff.h
bscand_LDFLAGS = -pthread 
bscand_LDADD = $(LIBCONFIG_LIBS)