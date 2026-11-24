MODULE_big = thai_parser
OBJS = src/thai_parser.o src/tokenizer.o

EXTENSION = thai_parser
DATA = sql/thai_parser--1.0.0.sql sql/thai_parser--1.1.0.sql sql/thai_parser--1.0.0--1.1.0.sql sql/thai_parser--unpackaged--1.0.0.sql

REGRESS = thai_parser
REGRESS_OPTS = --inputdir=tests --encoding=UTF8

LIBDIR = /usr/local/lib/
LIB_FLAGS = -lthai
PG_CPPFLAGS = -I$(LIBDIR)/include
SHLIB_LINK = $(LIB_FLAGS) -L$(LIBDIR)

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


test:
	gcc src/tokenizer.c tests/test_thai_parser.c $(LIB_FLAGS) -g -o tests/test_tp && ./tests/test_all.sh
