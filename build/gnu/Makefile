
LINK	= gcc
MAKE	=make
AR	= ar -r
LN	= ln -s
RM	= /bin/rm -rf
CP	= /bin/cp -p
MV	= /bin/mv -f

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"
PROJECTDIR = $(shell sh -c 'pwd')/../..
TARGET = $(PROJECTDIR)/bin/saker
USE_JEMALLOC = 1

#deps declare
HiredisDir=${PROJECTDIR}/deps/hiredis
LuaDir=${PROJECTDIR}/deps/lua
JemallocDir=${PROJECTDIR}/deps/jemalloc

#gcov declare
COVDIR=${PROJECTDIR}/tmp/lcov

#include declare
INCPATH	+= -I/usr/include
INCPATH	+= -I${PROJECTDIR}/deps
INCPATH	+= -I${LuaDir}/src
INCPATH	+= -I${JemallocDir}/include
INCPATH	+= -I${PROJECTDIR}/src

#static lib declare
AFLAGS += $(PROJECTDIR)/lib/liblua.a 
AFLAGS += $(PROJECTDIR)/lib/libhiredis.a 

ifeq (${DEBUG},1)
  USE_JEMALLOC = 0
  OPTIMIZATION=-O0
  CFLAGS += -g -ggdb
else
  OPTIMIZATION=-O2
  CFLAGS += -DNDEBUG 
endif

ifeq (${USE_JEMALLOC},1)
  AFLAGS += ${PROJECTDIR}/lib/libjemalloc.a
  #CFLAGS += -DUSE_JEMALLOC
endif


# Fallback to gcc when $CC is not in $PATH.
CC:=$(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')
OPTIMIZATION?=-O2
WARNINGS=-Wall -W -Wstrict-prototypes -Wwrite-strings
REAL_CFLAGS=$(OPTIMIZATION) -fPIC $(CFLAGS) $(COVERAGE_CFLAGS) $(WARNINGS) $(ARCH) -c $(INCPATH)
REAL_LDFLAGS=$(LDFLAGS) $(COVERAGE_LDFLAGS) $(ARCH)

#uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
#ifeq ($(uname_S),SunOS)
#  REAL_LDFLAGS+= -ldl -lnsl -lsocket
#  DYLIB_MAKE_CMD=$(CC) -G -o $(DYLIBNAME) -h $(DYLIB_MINOR_NAME) $(LDFLAGS)
# INSTALL= cp -r
#endif

#for lua
REAL_LDFLAGS += -lm
REAL_LDFLAGS += -pthread

OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/utils/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/core/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/service/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/sysinfo/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/sysinfo/linux/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/plugins/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/proto/*.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/event/zmalloc.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/event/adlist.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/event/ae.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/event/anet.c))
OBJECTS += $(patsubst %.c, %.o, $(wildcard ${PROJECTDIR}/src/event/util.c))

all: deps $(TARGET)
deps:
	$(MAKE) jemalloc
	$(MAKE) hiredis
	$(MAKE) lua
$(TARGET): $(OBJECTS)
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR)
	$(LINK) $(OBJECTS) $(LIBPATH) $(AFLAGS) $(REAL_LDFLAGS) -o $(TARGET) 
%.o:%.c
	$(CC) $(REAL_CFLAGS) $< -o $@

jemalloc:
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR)
	cd $(JemallocDir) && chmod +x configure && \
  find . -name "*" | xargs dos2unix -q && \
  find . -name "*.sh" | xargs chmod +x && \
  ./configure && $(MAKE) && \
  $(CP) $(JemallocDir)/lib/libjemalloc.a ${PROJECTDIR}/lib
hiredis:
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR)
	cd $(HiredisDir) && $(MAKE) static DEBUG="" && $(CP) ${PROJECTDIR}/deps/hiredis/libhiredis.a ${PROJECTDIR}/lib
lua:
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR)
	cd $(LuaDir) && $(MAKE) ansi && $(CP) ${PROJECTDIR}/deps/lua/src/liblua.a ${PROJECTDIR}/lib 
  
#gprof:
#	$(MAKE) DEBUG=1 CFLAGS="-pg" LDFLAGS="-pg"
  
gcov:
	$(MAKE) DEBUG=1 COVERAGE_CFLAGS="-fprofile-arcs -ftest-coverage" COVERAGE_LDFLAGS="-fprofile-arcs"

coverage: 
	mkdir -p $(COVDIR)
	lcov -d $(PROJECTDIR) -c -o $(COVDIR)/saker.info
	genhtml --legend -o ${COVDIR}/report $(COVDIR)/saker.info
  
clean:
	-($(RM) $(OBJECTS) $(TARGET)) 
	-(cd $(HiredisDir) && $(MAKE) clean) > /dev/null || true
	-(cd $(LuaDir) && $(MAKE) clean) > /dev/null || true
	-(cd $(JemallocDir) && $(MAKE) clean) > /dev/null || true
	-($(RM) $(PROJECTDIR)/lib/*.a) 
	-(cd $(PROJECTDIR) && find . -name "*.o" | xargs $(RM) ) 
	-(cd $(PROJECTDIR) && find . -name "*.gcda" | xargs $(RM) )
	-(cd $(PROJECTDIR) && find . -name "*.gcno" | xargs $(RM) )
	-(cd $(PROJECTDIR) && find . -name "*.gcov" | xargs $(RM) )
	-($(RM) $(COVDIR)) 
  
debug:
	make DEBUG=1
  
c:
	$(MAKE) clean -s
  
install:
	echo "install"
