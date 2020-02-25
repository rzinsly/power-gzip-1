ATDIR = /opt/at11.0
#OPTCC = $(ATDIR)/bin/gcc

## Common flags
FLG = -std=gnu11
SFLAGS = -O3 -fPIC -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN
ZLIB = -DZLIB_API

## Compiler related flags
ifneq ("$(wildcard $(OPTCC))","")
	CC = $(OPTCC)
	SFLAGS += -mcpu=power9
	ZLIB_PATH ?= $(ATDIR)/lib64/libz.so.1
else
	CC = gcc
	SFLAGS += -DNX_NO_CPU_PRI
	ZLIB_PATH ?= /lib64/libz.so.1
endif

CFLAGS += $(FLG) $(SFLAGS) $(ZLIB) -DZLIB_PATH=\"$(ZLIB_PATH)\" #-DNXTIMER

SRCS = nx_inflate.c nx_deflate.c nx_zlib.c nx_crc.c nx_dht.c nx_dhtgen.c nx_dht_builtin.c \
       nx_adler32.c gzip_vas.c nx_compress.c nx_uncompr.c crc32_ppc.c crc32_ppc_asm.S nx_utils.c sw_zlib.c
OBJS = nx_inflate.o nx_deflate.o nx_zlib.o nx_crc.o nx_dht.o nx_dhtgen.o nx_dht_builtin.o \
       nx_adler32.o gzip_vas.o nx_compress.o nx_uncompr.o crc32_ppc.o crc32_ppc_asm.o nx_utils.o sw_zlib.o

VERSION ?= $(shell git describe --tags | cut -d - -f 1,2 | tr - . | cut -c 2-)
SOVERSION = $(shell echo $(VERSION) | cut -d . -f 1)
PACKAGENAME = libnxz
STATICLIB = $(PACKAGENAME).a
SHAREDSONAMELIB = $(PACKAGENAME).so.$(SOVERSION)
SHAREDLIB = $(PACKAGENAME).so.$(VERSION)
LIBLINK = $(PACKAGENAME).so

INC = ./inc_nx

all: $(OBJS) $(STATICLIB) $(SHAREDLIB)

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -I$(INC) -c $^

$(STATICLIB): $(OBJS)
	rm -f $@
	ar rcs -o $@ $(OBJS)

$(SHAREDLIB): $(OBJS)
	rm -f $@ $(LIBLINK) 
	$(CC) -shared  -Wl,-soname,$(SHAREDSONAMELIB),--version-script,Versions -o $@ $(OBJS) -ldl
	ln -s $@ $(LIBLINK)
	ln -s $@ $(SHAREDSONAMELIB)

clean:
	/bin/rm -f *.o *.gcda *.gcno *.so* *.a *~
	$(MAKE) -C test $@

check:
	$(MAKE) -C test $@
