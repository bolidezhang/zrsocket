#****************************************************************************:
#
# Makefile for zrsocket
# bolide zhang
# bolidezhang@gmail.com
#
# This is a GNU make (gmake) makefile
#****************************************************************************

# DEBUG can be set to YES to include debugging info, or NO otherwise
DEBUG          := NO

# PROFILE can be set to YES to include profiling info, or NO otherwise
PROFILE        := NO

# USE_STL can be used to turn on STL support. NO, then STL
# will not be used. YES will include the STL files.
USE_STL := YES

# WIN32_ENV
WIN32_ENV := YES
#****************************************************************************

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

# ifeq (YES, ${WIN32_ENV})
#   RM     := del
# else
#   RM     := rm -f
# endif

DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -O3 -DNDEBUG

DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS}
RELEASE_CXXFLAGS := ${RELEASE_CFLAGS}

DEBUG_LDFLAGS    := -g
RELEASE_LDFLAGS  := -O3

ifeq (YES, ${DEBUG})
   CFLAGS       := ${DEBUG_CFLAGS}
   CXXFLAGS     := ${DEBUG_CXXFLAGS}
   LDFLAGS      := ${DEBUG_LDFLAGS}
else
   CFLAGS       := ${RELEASE_CFLAGS}
   CXXFLAGS     := ${RELEASE_CXXFLAGS}
   LDFLAGS      := ${RELEASE_LDFLAGS}
endif

ifeq (YES, ${PROFILE})
   CFLAGS   := ${CFLAGS} -pg -O3
   CXXFLAGS := ${CXXFLAGS} -pg -O3
   LDFLAGS  := ${LDFLAGS} -pg
endif

#****************************************************************************
# Preprocessor directives
#****************************************************************************

ifeq (YES, ${USE_STL})
  DEFS := -DUSE_STL
else
  DEFS :=
endif

#****************************************************************************
# Include paths
#****************************************************************************

#INCS := -I/usr/include/g++-2 -I/usr/local/include
INCS := -I/usr/local/include -I./include
LIBS := -L/usr/local/lib/ -lpthread

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS   := ${CFLAGS}   ${DEFS}
CXXFLAGS := ${CXXFLAGS} ${DEFS}

#****************************************************************************
# Targets of the build
#****************************************************************************

OUTPUT := libzrsocket.a

all: ${OUTPUT}


#****************************************************************************
# Source files
#****************************************************************************

SRCS := ./src/data_convert.cpp \
./src/inet_addr.cpp \
./src/memory.cpp \
./src/os_api.cpp \
./src/seda_timer_queue.cpp

# Add on the sources for libraries
SRCS := ${SRCS}

OBJS := $(addsuffix .o,$(basename ${SRCS}))

#****************************************************************************
# Output
#****************************************************************************

${OUTPUT}: ${OBJS}
#	${LD} -o $@ ${LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}
	$(AR) $@ $(OBJS)
	mkdir lib -p
	cp ${OUTPUT} ./lib -rf
#****************************************************************************
# common rules
#****************************************************************************

# Rules for compiling source files to object files
%.o : %.cpp
	${CXX} -c -std=c++11 ${CXXFLAGS} ${INCS} $< -o $@

%.o : %.c
	${CC} -c -std=c11 ${CFLAGS} ${INCS} $< -o $@

dist:
	bash makedistlinux

clean:
	${RM} core ${OBJS} ${OUTPUT}

depend:
	#makedepend ${INCS} ${SRCS}

%.o: %.h
