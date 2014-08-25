MODULE_big = thai_parser
OBJS = src/thai_parser.o src/converter.o

EXTENSION = thai_parser
DATA = sql/thai_parser--1.0.sql sql/thai_parser--unpackaged--1.0.sql

REGRESS = thai_parser

LIBDIR = /usr/local/lib/
LIB_FLAGS = -lthai
PG_CPPFLAGS = -I$(LIBDIR)/include
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
LIB_FLAGS += -liconv
endif
SHLIB_LINK = $(LIB_FLAGS) -L$(LIBDIR)

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


test:
	gcc src/converter.c tests/test_thai_parser.c $(LIB_FLAGS) -g -o tests/test_tp && ./tests/test_all.sh
